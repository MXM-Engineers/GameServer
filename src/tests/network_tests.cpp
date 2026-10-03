#include <common/base.h>
#include <common/network.h>
#include <common/protocol.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#ifdef CONF_LINUX
#include <cerrno>
#include <fcntl.h>
#endif

namespace
{
using Clock = std::chrono::steady_clock;
using Deadline = Clock::time_point;

bool Check(bool condition, const char* expression, int line)
{
	if(!condition) std::fprintf(stderr, "FAIL line %d: %s\n", line, expression);
	return condition;
}

#define REQUIRE(expression) do { if(!Check(bool(expression), #expression, __LINE__)) return false; } while(false)

Deadline After(int milliseconds)
{
	return Clock::now() + std::chrono::milliseconds(milliseconds);
}

void Pause(int milliseconds = 1)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

class Watchdog
{
	std::mutex mutex;
	std::condition_variable changed;
	bool finished = false;
	std::thread worker;

public:
	Watchdog(const char* operation, int milliseconds): worker([this, operation, milliseconds] {
		std::unique_lock<std::mutex> lock(mutex);
		if(!changed.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] { return finished; })) {
			std::fprintf(stderr, "FAIL timeout: %s (%d ms)\n", operation, milliseconds);
			std::fflush(stderr);
			std::_Exit(1);
		}
	}) {}

	~Watchdog()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			finished = true;
		}
		changed.notify_one();
		worker.join();
	}

	Watchdog(const Watchdog&) = delete;
	Watchdog& operator=(const Watchdog&) = delete;
};

bool WouldBlock(int error)
{
#ifdef CONF_WINDOWS
	return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
	return error == EWOULDBLOCK || error == EAGAIN || error == EINPROGRESS;
#endif
}

bool Interrupted(int error)
{
#ifdef CONF_WINDOWS
	return error == WSAEINTR;
#else
	return error == EINTR;
#endif
}

bool ConnectionClosed(int error)
{
#ifdef CONF_WINDOWS
	return error == WSAECONNRESET || error == WSAENOTCONN || error == WSAESHUTDOWN;
#else
	return error == ECONNRESET || error == ENOTCONN;
#endif
}

int SelectSocket(SOCKET socket, bool reading, Deadline deadline)
{
	for(;;) {
		auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - Clock::now()).count();
		if(remaining < 0) remaining = 0;
		timeval timeout{};
		timeout.tv_sec = long(remaining / 1000000);
		timeout.tv_usec = long(remaining % 1000000);
		fd_set ready;
		FD_ZERO(&ready);
		FD_SET(socket, &ready);
		fd_set exceptional = ready;
#ifdef CONF_WINDOWS
		const int result = select(0, reading ? &ready : nullptr, reading ? nullptr : &ready,
			reading ? nullptr : &exceptional, &timeout);
#else
		const int result = select(socket + 1, reading ? &ready : nullptr, reading ? nullptr : &ready,
			reading ? nullptr : &exceptional, &timeout);
#endif
		if(result != SOCKET_ERROR || !Interrupted(NetworkGetLastError())) return result;
		if(Clock::now() >= deadline) return 0;
	}
}

struct Packet
{
	u32 owner;
	u32 sequence;
	u16 payloadSize;
	u16 netID;

	Packet(u32 owner_, u32 sequence_, u16 payloadSize_): owner(owner_), sequence(sequence_),
		payloadSize(payloadSize_), netID(u16(1000 + (owner_ * 37 + sequence_ * 13) % 50000)) {}
};

u8 PayloadByte(const Packet& packet, size_t offset)
{
	if(offset < 4) return u8(packet.owner >> (offset * 8));
	if(offset < 8) return u8(packet.sequence >> ((offset - 4) * 8));
	const u32 mixed = packet.owner * 2654435761u + packet.sequence * 2246822519u + u32(offset) * 3266489917u;
	return u8(mixed ^ (mixed >> 11) ^ (mixed >> 19));
}

void FillPayload(const Packet& packet, u8* out)
{
	for(size_t offset = 0; offset < packet.payloadSize; ++offset) out[offset] = PayloadByte(packet, offset);
}

void AppendPacket(std::vector<u8>& bytes, const Packet& packet)
{
	const size_t begin = bytes.size();
	bytes.resize(begin + sizeof(NetHeader) + packet.payloadSize);
	const NetHeader header{u16(sizeof(NetHeader) + packet.payloadSize), packet.netID};
	std::memcpy(bytes.data() + begin, &header, sizeof(header));
	FillPayload(packet, bytes.data() + begin + sizeof(header));
}

bool VerifyPayload(const Packet& packet, const u8* bytes)
{
	for(size_t offset = 0; offset < packet.payloadSize; ++offset) {
		if(bytes[offset] != PayloadByte(packet, offset)) {
			std::fprintf(stderr, "FAIL payload owner=%u sequence=%u offset=%zu: got=%u expected=%u\n",
				packet.owner, packet.sequence, offset, unsigned(bytes[offset]), unsigned(PayloadByte(packet, offset)));
			return false;
		}
	}
	return true;
}

class Peer
{
	SOCKET socket = INVALID_SOCKET;

public:
	ClientHandle handle = ClientHandle::INVALID;

	Peer() = default;
	~Peer() { Close(); }
	Peer(const Peer&) = delete;
	Peer& operator=(const Peer&) = delete;
	Peer(Peer&& other) noexcept: socket(other.socket), handle(other.handle)
	{
		other.socket = INVALID_SOCKET;
		other.handle = ClientHandle::INVALID;
	}

	Peer& operator=(Peer&& other) noexcept
	{
		if(this != &other) {
			Close();
			socket = other.socket;
			handle = other.handle;
			other.socket = INVALID_SOCKET;
			other.handle = ClientHandle::INVALID;
		}
		return *this;
	}

	void Close(bool reset = false)
	{
		if(socket == INVALID_SOCKET) return;
		if(reset) {
			linger option{};
			option.l_onoff = 1;
			option.l_linger = 0;
			setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&option), sizeof(option));
		}
		closesocket(socket);
		socket = INVALID_SOCKET;
	}

	bool Connect(u16 port, int receiveWindow = 0, int* connectionError = nullptr)
	{
		Close();
		if(connectionError) *connectionError = 0;
		socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(socket == INVALID_SOCKET) return false;
		if(receiveWindow && setsockopt(socket, SOL_SOCKET, SO_RCVBUF,
			reinterpret_cast<const char*>(&receiveWindow), sizeof(receiveWindow)) == SOCKET_ERROR) return false;
#ifdef CONF_WINDOWS
		u_long nonblocking = 1;
		if(ioctlsocket(socket, FIONBIO, &nonblocking) == SOCKET_ERROR) return false;
#else
		const int flags = fcntl(socket, F_GETFL, 0);
		if(flags == -1 || fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) return false;
#endif
		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		address.sin_port = htons(port);
		const int result = connect(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
		if(result == 0) return true;
		const int error = NetworkGetLastError();
		if(!WouldBlock(error)) {
			if(connectionError) *connectionError = error;
			return false;
		}
		if(SelectSocket(socket, false, After(2000)) <= 0) return false;
		int completedError = 0;
		AddrLen length = sizeof(completedError);
		if(getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&completedError), &length) == SOCKET_ERROR) return false;
		if(connectionError) *connectionError = completedError;
		return completedError == 0;
	}

	bool Send(const u8* bytes, size_t count, int timeoutMilliseconds = 3000)
	{
		const Deadline deadline = After(timeoutMilliseconds);
		size_t sent = 0;
		while(sent < count && Clock::now() < deadline) {
#ifdef CONF_LINUX
			const int flags = MSG_NOSIGNAL;
#else
			const int flags = 0;
#endif
			const int result = send(socket, reinterpret_cast<const char*>(bytes + sent),
				int(std::min<size_t>(count - sent, 32768)), flags);
			if(result > 0) {
				sent += size_t(result);
				continue;
			}
			if(result == 0) return false;
			const int error = NetworkGetLastError();
			if(Interrupted(error)) continue;
			if(!WouldBlock(error) || SelectSocket(socket, false, deadline) <= 0) return false;
		}
		return sent == count;
	}

	bool Read(u8* bytes, size_t count, Deadline deadline, bool slow = false)
	{
		size_t received = 0;
		while(received < count && Clock::now() < deadline) {
			const int result = recv(socket, reinterpret_cast<char*>(bytes + received),
				int(std::min<size_t>(count - received, slow ? 1024 : 32768)), 0);
			if(result > 0) {
				received += size_t(result);
				continue;
			}
			if(result == 0) return false;
			const int error = NetworkGetLastError();
			if(Interrupted(error)) continue;
			if(!WouldBlock(error) || SelectSocket(socket, true, deadline) <= 0) return false;
		}
		return received == count;
	}

	bool ReadPacket(const Packet& expected, Deadline deadline, bool slow = false)
	{
		NetHeader header{};
		REQUIRE(Read(reinterpret_cast<u8*>(&header), sizeof(header), deadline));
		REQUIRE(header.size == sizeof(NetHeader) + expected.payloadSize);
		REQUIRE(header.netID == expected.netID);
		std::array<u8, SEND_BUFF_LEN> payload;
		REQUIRE(expected.payloadSize <= payload.size());
		REQUIRE(Read(payload.data(), expected.payloadSize, deadline, slow));
		if(slow) Pause();
		REQUIRE(VerifyPayload(expected, payload.data()));
		return true;
	}

	bool NoData(int milliseconds)
	{
		return SelectSocket(socket, true, After(milliseconds)) == 0;
	}

	bool WaitClosed(int milliseconds)
	{
		const Deadline deadline = After(milliseconds);
		std::array<char, RECV_BUFF_LEN> discard;
		while(Clock::now() < deadline) {
			const int result = recv(socket, discard.data(), int(discard.size()), 0);
			if(result == 0) return true;
			if(result > 0) continue;
			const int error = NetworkGetLastError();
			if(ConnectionClosed(error)) return true;
			if(Interrupted(error)) continue;
			if(!WouldBlock(error) || SelectSocket(socket, true, deadline) <= 0) return false;
		}
		return false;
	}
};

struct Fixture
{
	std::unique_ptr<Server> server{new Server()};
	Listener listener{server.get()};
	std::thread listenerThread;
	u16 port = 0;
	bool initialized = false;
	bool listening = false;

	Fixture() { listener.listenSocket = INVALID_SOCKET; }
	~Fixture() { Shutdown(); }
	Fixture(const Fixture&) = delete;
	Fixture& operator=(const Fixture&) = delete;

	bool Start()
	{
		REQUIRE(server->Init());
		initialized = true;
		REQUIRE(listener.Init(0));
		listening = true;
		sockaddr_in address{};
		AddrLen length = sizeof(address);
		REQUIRE(getsockname(listener.listenSocket, reinterpret_cast<sockaddr*>(&address), &length) == 0);
		port = ntohs(address.sin_port);
		REQUIRE(port != 0);
		listenerThread = std::thread([this] { listener.Listen(); });
		return true;
	}

	void Shutdown()
	{
		if(!initialized) return;
		Watchdog watchdog("listener stop and completion drain", 5000);
		server->Stop();
		if(listening || listener.listenSocket != INVALID_SOCKET) {
			listener.Stop();
			listening = false;
		}
		if(listenerThread.joinable()) listenerThread.join();
		server->Cleanup();
		initialized = false;
	}

	bool Connect(Peer& peer, int receiveWindow = 0)
	{
		REQUIRE(peer.Connect(port, receiveWindow));
		std::vector<ClientHandle> connected;
		const Deadline deadline = After(3000);
		while(Clock::now() < deadline) {
			server->TransferConnectedClientList(&connected);
			if(!connected.empty()) {
				REQUIRE(connected.size() == 1);
				REQUIRE(connected[0] != ClientHandle::INVALID);
				peer.handle = connected[0];
				return true;
			}
			Pause();
		}
		return Check(false, "connection notification before deadline", __LINE__);
	}

	bool Disconnected(ClientHandle expected)
	{
		std::vector<ClientHandle> disconnected;
		const Deadline deadline = After(3000);
		while(Clock::now() < deadline) {
			server->TransferDisconnectedClientList(&disconnected);
			if(!disconnected.empty()) {
				REQUIRE(disconnected.size() == 1);
				REQUIRE(disconnected[0] == expected);
				return true;
			}
			Pause();
		}
		return Check(false, "disconnection notification before deadline", __LINE__);
	}

	void Send(ClientHandle handle, const Packet& packet)
	{
		std::array<u8, SEND_BUFF_LEN> payload;
		FillPayload(packet, payload.data());
		server->SendPacketData(handle, packet.netID, packet.payloadSize, payload.data());
	}
};

class Inbox
{
	struct Stream
	{
		std::vector<Packet> expected;
		size_t received = 0;
	};

	std::map<u32, Stream> streams;
	GrowableBuffer transferred{16384};
	size_t received = 0;

public:
	void Expect(ClientHandle handle, const Packet& packet)
	{
		streams[u32(handle)].expected.push_back(packet);
	}

	size_t Count() const { return received; }

	bool Drain(Server& server, const std::vector<ClientHandle>* selection = nullptr)
	{
		transferred.Clear();
		if(selection) server.TransferReceivedData(&transferred, selection->data(), u32(selection->size()));
		else server.TransferAllReceivedData(&transferred);
		size_t cursor = 0;
		while(cursor < size_t(transferred.size)) {
			REQUIRE(size_t(transferred.size) - cursor >= sizeof(Server::RecvChunkHeader));
			Server::RecvChunkHeader chunk{};
			std::memcpy(&chunk, transferred.data + cursor, sizeof(chunk));
			cursor += sizeof(chunk);
			REQUIRE(chunk.len > 0);
			REQUIRE(size_t(chunk.len) <= size_t(transferred.size) - cursor);
			if(selection) REQUIRE(std::find(selection->begin(), selection->end(), chunk.clientHd) != selection->end());
			auto found = streams.find(u32(chunk.clientHd));
			REQUIRE(found != streams.end());
			Stream& stream = found->second;
			const size_t end = cursor + size_t(chunk.len);
			while(cursor < end) {
				REQUIRE(end - cursor >= sizeof(NetHeader));
				NetHeader header{};
				std::memcpy(&header, transferred.data + cursor, sizeof(header));
				REQUIRE(header.size >= sizeof(NetHeader));
				REQUIRE(header.size <= end - cursor);
				REQUIRE(stream.received < stream.expected.size());
				const Packet& expected = stream.expected[stream.received];
				REQUIRE(header.size == sizeof(NetHeader) + expected.payloadSize);
				REQUIRE(header.netID == expected.netID);
				REQUIRE(VerifyPayload(expected, transferred.data + cursor + sizeof(header)));
				cursor += header.size;
				++stream.received;
				++received;
			}
		}
		return true;
	}

	bool Await(Server& server, size_t count, int milliseconds = 3000,
		const std::vector<ClientHandle>* selection = nullptr)
	{
		const Deadline deadline = After(milliseconds);
		do {
			REQUIRE(Drain(server, selection));
			REQUIRE(received <= count);
			if(received == count) return true;
			Pause();
		} while(Clock::now() < deadline);
		std::fprintf(stderr, "FAIL receive timeout: got=%zu expected=%zu\n", received, count);
		return false;
	}

	bool Quiet(Server& server, int milliseconds = 40,
		const std::vector<ClientHandle>* selection = nullptr)
	{
		const size_t before = received;
		const Deadline deadline = After(milliseconds);
		do {
			REQUIRE(Drain(server, selection));
			REQUIRE(received == before);
			Pause();
		} while(Clock::now() < deadline);
		return true;
	}
};

bool FragmentedAndCoalescedFrames()
{
	Fixture fixture;
	REQUIRE(fixture.Start());
	Peer peer;
	REQUIRE(fixture.Connect(peer));
	Inbox inbox;
	const Packet first(17, 0, 347);
	inbox.Expect(peer.handle, first);
	std::vector<u8> bytes;
	AppendPacket(bytes, first);
	REQUIRE(peer.Send(bytes.data(), 1));
	REQUIRE(inbox.Quiet(*fixture.server));
	REQUIRE(peer.Send(bytes.data() + 1, 2));
	REQUIRE(inbox.Quiet(*fixture.server));
	REQUIRE(peer.Send(bytes.data() + 3, 102));
	REQUIRE(inbox.Quiet(*fixture.server));
	REQUIRE(peer.Send(bytes.data() + 105, bytes.size() - 106));
	REQUIRE(inbox.Quiet(*fixture.server));
	REQUIRE(peer.Send(bytes.data() + bytes.size() - 1, 1));
	REQUIRE(inbox.Await(*fixture.server, 1));

	const Packet empty(17, 1, 0);
	const Packet small(17, 2, 8);
	const Packet spanning(17, 3, u16(RECV_BUFF_LEN + 117));
	const Packet tail(17, 4, 91);
	inbox.Expect(peer.handle, empty);
	inbox.Expect(peer.handle, small);
	inbox.Expect(peer.handle, spanning);
	inbox.Expect(peer.handle, tail);
	bytes.clear();
	AppendPacket(bytes, empty);
	AppendPacket(bytes, small);
	const size_t spanningBegin = bytes.size();
	AppendPacket(bytes, spanning);
	AppendPacket(bytes, tail);
	REQUIRE(peer.Send(bytes.data(), spanningBegin + 2));
	REQUIRE(inbox.Await(*fixture.server, 3));
	REQUIRE(inbox.Quiet(*fixture.server));
	REQUIRE(peer.Send(bytes.data() + spanningBegin + 2, 517));
	REQUIRE(inbox.Quiet(*fixture.server));
	REQUIRE(peer.Send(bytes.data() + spanningBegin + 519, 4093));
	REQUIRE(inbox.Quiet(*fixture.server));
	REQUIRE(peer.Send(bytes.data() + spanningBegin + 4612, bytes.size() - spanningBegin - 4612));
	REQUIRE(inbox.Await(*fixture.server, 5));
	REQUIRE(inbox.Quiet(*fixture.server));
	return true;
}

bool TwoHundredIndependentPeers()
{
	Fixture fixture;
	REQUIRE(fixture.Start());
	const size_t peerCount = 200;
	const size_t silentCount = 40;
	std::vector<Peer> peers;
	peers.reserve(peerCount);
	std::vector<ClientHandle> even;
	std::vector<ClientHandle> odd;
	Inbox inbox;
	for(size_t index = 0; index < peerCount; ++index) {
		peers.emplace_back();
		REQUIRE(fixture.Connect(peers.back()));
		for(size_t previous = 0; previous < index; ++previous) REQUIRE(peers[previous].handle != peers[index].handle);
		if(index < silentCount) continue;
		const u32 owner = u32(100 + index);
		inbox.Expect(peers[index].handle, Packet(owner, 0, u16(17 + index % 37)));
		inbox.Expect(peers[index].handle, Packet(owner, 1, u16(300 + index % 29)));
		inbox.Expect(peers[index].handle, Packet(owner, 2, u16(4093 + index % 71)));
		(index % 2 ? odd : even).push_back(peers[index].handle);
	}
	std::vector<u8> bytes;
	for(size_t index = silentCount; index < peerCount; ++index) {
		bytes.clear();
		AppendPacket(bytes, Packet(u32(100 + index), 0, u16(17 + index % 37)));
		REQUIRE(peers[index].Send(bytes.data(), 1 + index % 3));
	}
	REQUIRE(inbox.Quiet(*fixture.server));
	for(size_t index = silentCount; index < peerCount; ++index) {
		bytes.clear();
		const u32 owner = u32(100 + index);
		AppendPacket(bytes, Packet(owner, 0, u16(17 + index % 37)));
		AppendPacket(bytes, Packet(owner, 1, u16(300 + index % 29)));
		AppendPacket(bytes, Packet(owner, 2, u16(4093 + index % 71)));
		const size_t sent = 1 + index % 3;
		REQUIRE(peers[index].Send(bytes.data() + sent, bytes.size() - sent));
	}
	REQUIRE(inbox.Await(*fixture.server, even.size() * 3, 5000, &even));
	REQUIRE(inbox.Quiet(*fixture.server, 30, &even));
	REQUIRE(inbox.Await(*fixture.server, (peerCount - silentCount) * 3, 5000, &odd));
	REQUIRE(inbox.Quiet(*fixture.server));

	for(size_t index = silentCount; index < peerCount; ++index) {
		fixture.Send(peers[index].handle, Packet(u32(900 + index), 0, u16(257 + index % 31)));
		fixture.Send(peers[index].handle, Packet(u32(900 + index), 1, u16(9 + index % 23)));
	}
	const Deadline deadline = After(5000);
	for(size_t index = silentCount; index < peerCount; ++index) {
		REQUIRE(peers[index].ReadPacket(Packet(u32(900 + index), 0, u16(257 + index % 31)), deadline));
		REQUIRE(peers[index].ReadPacket(Packet(u32(900 + index), 1, u16(9 + index % 23)), deadline));
		REQUIRE(peers[index].NoData(0));
	}
	for(size_t index = 0; index < silentCount; ++index) REQUIRE(peers[index].NoData(0));
	std::vector<ClientHandle> disconnected;
	fixture.server->TransferDisconnectedClientList(&disconnected);
	REQUIRE(disconnected.empty());
	return true;
}

bool BackpressurePreservesEveryByte()
{
	Fixture fixture;
	REQUIRE(fixture.Start());
	Peer slow;
	Peer fast;
	REQUIRE(fixture.Connect(slow, 16384));
	REQUIRE(fixture.Connect(fast));
	const u32 packetCount = 640;
	const u16 payloadSize = u16(SEND_BUFF_LEN - sizeof(NetHeader));
	for(u32 sequence = 0; sequence < 64; ++sequence) fixture.Send(slow.handle, Packet(4001, sequence, payloadSize));
	Pause(100);
	for(u32 sequence = 64; sequence < packetCount; ++sequence) {
		fixture.Send(slow.handle, Packet(4001, sequence, payloadSize));
		if(sequence == 320) fixture.Send(fast.handle, Packet(4002, 0, 193));
	}
	fixture.Send(fast.handle, Packet(4002, 1, 327));
	const Deadline fastDeadline = After(3000);
	REQUIRE(fast.ReadPacket(Packet(4002, 0, 193), fastDeadline));
	REQUIRE(fast.ReadPacket(Packet(4002, 1, 327), fastDeadline));
	REQUIRE(fast.NoData(0));
	Pause(100);
	const Deadline slowDeadline = After(20000);
	for(u32 sequence = 0; sequence < packetCount; ++sequence) {
		REQUIRE(slow.ReadPacket(Packet(4001, sequence, payloadSize), slowDeadline, true));
		if(sequence == 127) {
			fixture.Send(fast.handle, Packet(4002, 2, 521));
			REQUIRE(fast.ReadPacket(Packet(4002, 2, 521), After(3000)));
		}
	}
	REQUIRE(slow.NoData(40));
	REQUIRE(fast.NoData(0));
	Inbox inbox;
	const Packet stillAlive(4001, 1000, 719);
	inbox.Expect(slow.handle, stillAlive);
	std::vector<u8> bytes;
	AppendPacket(bytes, stillAlive);
	REQUIRE(slow.Send(bytes.data(), bytes.size()));
	REQUIRE(inbox.Await(*fixture.server, 1));
	return true;
}

bool PendingDisconnectAndSlotReuse()
{
	Fixture fixture;
	REQUIRE(fixture.Start());
	std::vector<Peer> peers;
	peers.reserve(MAX_CLIENTS);
	for(size_t index = 0; index < size_t(MAX_CLIENTS); ++index) {
		peers.emplace_back();
		REQUIRE(fixture.Connect(peers.back(), 16384));
	}
	std::vector<ClientHandle> retired;
	Inbox inbox;
	std::vector<u8> bytes;
	for(u32 round = 0; round < 8; ++round) {
		const size_t victim = round;
		const ClientHandle oldHandle = peers[victim].handle;
		bytes.clear();
		AppendPacket(bytes, Packet(5000 + round, 0, 3000));
		REQUIRE(peers[victim].Send(bytes.data(), 1003));
		REQUIRE(inbox.Quiet(*fixture.server, 30));
		for(u32 sequence = 0; sequence < 320; ++sequence) {
			fixture.Send(oldHandle, Packet(6000 + round, sequence, u16(SEND_BUFF_LEN - sizeof(NetHeader))));
		}
		Pause(30);
		if(round % 2 == 0) peers[victim].Close(true);
		else fixture.server->DisconnectClient(oldHandle);
		REQUIRE(fixture.Disconnected(oldHandle));
		if(round % 2) REQUIRE(peers[victim].WaitClosed(3000));
		peers[victim].Close(true);
		retired.push_back(oldHandle);
		REQUIRE(fixture.Connect(peers[victim], 16384));
		REQUIRE(std::find(retired.begin(), retired.end(), peers[victim].handle) == retired.end());
		for(ClientHandle stale: retired) {
			fixture.Send(stale, Packet(7000 + round, 0, 113));
			fixture.server->DisconnectClient(stale);
		}
		const Packet reply(8000 + round, 0, 677);
		fixture.Send(peers[victim].handle, reply);
		REQUIRE(peers[victim].ReadPacket(reply, After(3000)));
		REQUIRE(peers[victim].NoData(30));
		const Packet incoming(9000 + round, 0, 2053);
		inbox.Expect(peers[victim].handle, incoming);
		bytes.clear();
		AppendPacket(bytes, incoming);
		REQUIRE(peers[victim].Send(bytes.data(), bytes.size()));
		REQUIRE(inbox.Quiet(*fixture.server, 100, &retired));
		const std::vector<ClientHandle> current{peers[victim].handle};
		REQUIRE(inbox.Await(*fixture.server, round + 1, 3000, &current));
		REQUIRE(inbox.Quiet(*fixture.server, 10, &retired));
		std::vector<ClientHandle> unexpected;
		fixture.server->TransferDisconnectedClientList(&unexpected);
		REQUIRE(unexpected.empty());
	}
	for(size_t index = 0; index < peers.size(); ++index) {
		const Packet incoming(u32(10000 + index), 0, u16(49 + index % 43));
		inbox.Expect(peers[index].handle, incoming);
		bytes.clear();
		AppendPacket(bytes, incoming);
		REQUIRE(peers[index].Send(bytes.data(), bytes.size()));
		fixture.Send(peers[index].handle, Packet(u32(11000 + index), 0, u16(83 + index % 29)));
	}
	REQUIRE(inbox.Await(*fixture.server, 8 + peers.size(), 5000));
	const Deadline deadline = After(5000);
	for(size_t index = 0; index < peers.size(); ++index) {
		REQUIRE(peers[index].ReadPacket(Packet(u32(11000 + index), 0, u16(83 + index % 29)), deadline));
		REQUIRE(peers[index].NoData(0));
	}
	REQUIRE(inbox.Quiet(*fixture.server, 30, &retired));
	return true;
}

bool OnePendingShutdown(u32 iteration)
{
	Fixture fixture;
	REQUIRE(fixture.Start());
	std::vector<Peer> peers;
	peers.reserve(8);
	for(u32 index = 0; index < 8; ++index) {
		peers.emplace_back();
		REQUIRE(fixture.Connect(peers.back(), 16384));
	}
	Inbox inbox;
	std::vector<u8> bytes;
	AppendPacket(bytes, Packet(12000 + iteration, 0, 2057));
	REQUIRE(peers[2].Send(bytes.data(), 2));
	REQUIRE(peers[3].Send(bytes.data(), bytes.size() - 1));
	REQUIRE(inbox.Quiet(*fixture.server, 30));
	for(u32 sequence = 0; sequence < 384; ++sequence) {
		fixture.Send(peers[0].handle, Packet(13000 + iteration, sequence, u16(SEND_BUFF_LEN - sizeof(NetHeader))));
		fixture.Send(peers[1].handle, Packet(14000 + iteration, sequence, u16(SEND_BUFF_LEN - sizeof(NetHeader))));
	}
	Pause(50);
	const Deadline begin = Clock::now();
	fixture.Shutdown();
	REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - begin).count() < 5000);
	REQUIRE(!fixture.server->running);
	for(Peer& peer: peers) REQUIRE(peer.WaitClosed(3000));
	Peer rejected;
	REQUIRE(!rejected.Connect(fixture.port));
	fixture.Shutdown();
	return true;
}

bool BoundedShutdownAndResourceCleanup()
{
	REQUIRE(OnePendingShutdown(0));
#ifdef CONF_WINDOWS
	DWORD before = 0;
	REQUIRE(GetProcessHandleCount(GetCurrentProcess(), &before));
#endif
	for(u32 iteration = 1; iteration <= 8; ++iteration) REQUIRE(OnePendingShutdown(iteration));
#ifdef CONF_WINDOWS
	DWORD after = 0;
	REQUIRE(GetProcessHandleCount(GetCurrentProcess(), &after));
	if(after > before + 2) {
		std::fprintf(stderr, "FAIL native handle leak: before=%lu after=%lu\n", before, after);
		return false;
	}
#endif
	return true;
}

struct NetworkRuntime
{
	bool initialized = NetworkInit();
	~NetworkRuntime() { if(initialized) NetworkCleanup(); }
};
}

int main()
{
	TimeInit();
	LogInit("network_tests.log", 0);
	NetworkRuntime network;
	if(!network.initialized) {
		std::fprintf(stderr, "NetworkInit failed: %d\n", NetworkGetLastError());
		return 1;
	}
	const struct { const char* name; bool (*run)(); } tests[] = {
		{"fragmented headers, payloads and coalesced complete frames", FragmentedAndCoalescedFrames},
		{"200 simultaneous peers, silent peers and independent routing", TwoHundredIndependentPeers},
		{"multi-megabyte backpressure, every byte and packet order", BackpressurePreservesEveryByte},
		{"pending read/send disconnects, full-capacity slot reuse and stale handles", PendingDisconnectAndSlotReuse},
		{"bounded pending-operation shutdown and native resource cleanup", BoundedShutdownAndResourceCleanup},
	};
	int failed = 0;
	for(const auto& test: tests) {
		std::printf("RUN %s\n", test.name);
		std::fflush(stdout);
		Watchdog watchdog(test.name, 60000);
		const bool passed = test.run();
		std::printf("%s %s\n", passed ? "PASS" : "FAIL", test.name);
		if(!passed) ++failed;
	}
	std::printf("network_tests: %d failed\n", failed);
	return failed ? 1 : 0;
}
