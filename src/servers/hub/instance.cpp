#include "instance.h"
#include "account.h"

bool HubInstance::Init(Server* server_)
{
	game.Init(server_, &plidMap);
	packetHandler.Init(&game);
	return true;
}

void HubInstance::Update(Time localTime_)
{
	game.Update(localTime_);
}

void HubInstance::OnClientsConnected(const NewUser* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		plidMap.Push(clientList[i].clientHd);
	}

	// FIXME: we should pull account data at the last moment and just pass AccountUIDs
	const AccountManager& am = GetAccountManager();
	eastl::fixed_vector<eastl::pair<ClientHandle, const Account*>,MAX_CLIENTS> list;
	for(int i = 0; i < count; i++) {
		list.push_back({ clientList[i].clientHd, &*am.accountMap.at(clientList[i].accountUID) });
	}

	packetHandler.OnClientsConnected(list.data(), count);
}

void HubInstance::OnClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	packetHandler.OnClientsDisconnected(clientList, count);

	for(int i = 0; i < count; i++) {
		plidMap.Pop(clientList[i]);
	}
}

void HubInstance::OnClientsTransferOut(const ClientHandle* clientList, const i32 count)
{
	// TODO: push to a 'transfer list' that will call replicate.SendClearAllEntities() before detaching clients
	packetHandler.OnClientsDisconnected(clientList, count);

	for(int i = 0; i < count; i++) {
		plidMap.Pop(clientList[i]);
	}
}

void HubInstance::OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	packetHandler.OnNewPacket(clientHd, header, packetData);
}

void HubInstance::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{
	packetHandler.OnMatchmakerPacket(header, packetData);
}

void RoomInstance::Init(Server* server_, const NewUser* userlist, const i32 userCount)
{
	LOG("[Room_%llx] room created (userCount=%d)", sortieUID, userCount);
}

void RoomInstance::Update(Time localTime_)
{

}

void RoomInstance::OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{

}

void RoomInstance::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{

}
