#include <common/base.h>
#include <EAStdC/EAString.h>
#include <EASTL/fixed_map.h>
#include <PxFoundation.h>
#include <PxPhysicsVersion.h>
#include <cooking/PxCooking.h>
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultErrorCallback.h>
#include <extensions/PxDefaultStreams.h>

using namespace physx;

#ifdef CONF_DEBUG
	#define DBG LOG
#else
	#define DBG
#endif

template<typename T>
inline T littleToBigEndian(T in)
{
	T out = 0x0;
	for(size_t i = 0; i < sizeof(T); i++) {
		out |= ((in >> (i * 8)) & 0xFF) << ((sizeof(T) - i - 1) * 8);
	}
	return out;
}

inline u32 littleToBigEndian(u32 in)
{
	u32 b0 = (in & 0x000000ff) << 24u;
	u32 b1 = (in & 0x0000ff00) << 8u;
	u32 b2 = (in & 0x00ff0000) >> 8u;
	u32 b3 = (in & 0xff000000) >> 24u;
	return b0 | b1 | b2 | b3;
}

struct Vector3
{
	f32 x,y,z;
};

struct Vector4
{
	f32 x,y,z,w;
};

struct Matrix3x3
{
	f32 data[9];
};

struct CollisionNifReader
{
	const u8* fileData;
	const i32 fileSize;

	struct StringSlice
	{
		const char* at;
		u32 len;

		inline bool Equals(const char* str) const
		{
			const i32 otherLen = strlen(str);
			if(otherLen != len) return false;
			return strncmp(at, str, otherLen) == 0;
		}

		inline bool StartsWith(const char* str) const
		{
			const i32 otherLen = strlen(str);
			if(otherLen > len) return false;
			return strncmp(at, str, otherLen) == 0;
		}
	};

	struct Mesh
	{
		struct DataStream
		{
			struct Component
			{
				StringSlice name;
				u32 index;
			};

			i32 refStream;
			u8 bIsPerInstance;
			eastl::fixed_vector<u16,16,true> subMeshToRegionMapList;
			eastl::fixed_vector<Component,16,true> componentList;
		};

		eastl::fixed_string<char,64,false> name;
		/*
			<option value="0" name="MESH_PRIMITIVE_TRIANGLES">Triangle primitive type.</option>
			<option value="1" name="MESH_PRIMITIVE_TRISTRIPS">Triangle strip primitive type.</option>
			<option value="2" name="MESH_PRIMITIVE_LINES">Lines primitive type.</option>
			<option value="3" name="MESH_PRIMITIVE_LINESTRIPS">Line strip primitive type.</option>
			<option value="4" name="MESH_PRIMITIVE_QUADS">Quadrilateral primitive type.</option>
			<option value="5" name="MESH_PRIMITIVE_POINTS">Point primitive type.</option>
		*/
		u32 meshPrimitiveType;
		eastl::fixed_vector<DataStream,16,true> dataStreamList;
	};

	struct DataStream
	{
		const void* at;
		u32 size;
		u32 numIndices;
	};

	struct Node
	{
		eastl::fixed_string<char,64,false> name;
	};

	eastl::fixed_vector<StringSlice,1024,true> stringList;
	eastl::fixed_vector<u16,1024,true> blockSizeList;
	eastl::fixed_map<i32,DataStream,16,true> dataStreamMap;

	eastl::fixed_vector<Mesh,8,true> meshList;
	Node currentNode;

	bool ReadBlock_NiNode(ConstBuffer buff, i32 blockID)
	{
		const i32 stringIndex = buff.Read<i32>();
		const StringSlice& str = (stringIndex != -1) ? stringList[stringIndex] : StringSlice{"_", 1};
		currentNode.name.assign(str.at, str.len);

		const u32 numExtraDataList = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refExtraDataList;
		for(int i = 0; i < numExtraDataList; i++) {
			refExtraDataList.push_back(buff.Read<i32>());
		}

		const i32 refController = buff.Read<i32>();
		const u16 flags = buff.Read<u16>();
		const Vector3 translation = buff.Read<Vector3>();
		const Matrix3x3 rotation = buff.Read<Matrix3x3>();
		const f32 scale = buff.Read<f32>();

		const u32 numProperties = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refPropertyList;
		for(int i = 0; i < numProperties; i++) {
			refPropertyList.push_back(buff.Read<i32>());
		}

		const i32 refCollisionObject = buff.Read<i32>();

		const u32 numChildren = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refChildrenList;
		for(int i = 0; i < numChildren; i++) {
			refChildrenList.push_back(buff.Read<i32>());
		}

		const u32 numEffects = buff.Read<u32>();

		LOG("	name = '%.*s'", str.len, str.at);
		LOG("	extraDataList(%d) = [", numExtraDataList);
		foreach(it, refExtraDataList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	refController = %d", refController);
		LOG("	flags = %#x", flags);
		LOG("	translation = (%g, %g, %g)", translation.x, translation.y, translation.z);
		LOG("	rotation = (%.2f, %.2f, %.2f", rotation.data[0], rotation.data[1], rotation.data[2]);
		LOG("	            %.2f, %.2f, %.2f", rotation.data[3], rotation.data[4], rotation.data[5]);
		LOG("	            %.2f, %.2f, %.2f)", rotation.data[6], rotation.data[7], rotation.data[8]);
		LOG("	scale = %g", scale);
		LOG("	properties(%d) = [", numProperties);
		foreach(it, refPropertyList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	refCollisionObject = %d", refCollisionObject);
		LOG("	children(%d) = [", numChildren);
		foreach(it, refChildrenList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	numEffects = %u", numEffects);
		return true;
	}

	bool ReadBlock_NiStringExtraData(ConstBuffer buff, i32 blockID)
	{
		const u32 nameStrID = buff.Read<u32>();
		const StringSlice& name = (nameStrID != 0xFFFFFFFF) ? stringList[nameStrID] : StringSlice{"_", 1};
		const u32 strDataStrID = buff.Read<u32>();
		const StringSlice& strData = (strDataStrID != 0xFFFFFFFF) ? stringList[strDataStrID] : StringSlice{"_", 1};

		LOG("	name = '%.*s'", name.len, name.at);
		LOG("	strData = '%.*s'", strData.len, strData.at);
		return true;
	}

	bool ReadBlock_NiMesh(ConstBuffer buff, i32 blockID)
	{
		Mesh& mesh = meshList.push_back();
		const u32 nameStrID = buff.Read<u32>();
		const StringSlice& name = (nameStrID != 0xFFFFFFFF) ? stringList[nameStrID] : StringSlice{"_", 1};
		mesh.name = currentNode.name;

		const u32 numExtraDataList = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refExtraDataList;
		for(int i = 0; i < numExtraDataList; i++) {
			refExtraDataList.push_back(buff.Read<i32>());
		}

		const i32 refController = buff.Read<i32>();
		const u16 flags = buff.Read<u16>();
		const Vector3 translation = buff.Read<Vector3>();
		const Matrix3x3 rotation = buff.Read<Matrix3x3>();
		const f32 scale = buff.Read<f32>();

		const u32 numProperties = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refPropertyList;
		for(int i = 0; i < numProperties; i++) {
			refPropertyList.push_back(buff.Read<i32>());
		}

		const i32 refCollisionObject = buff.Read<i32>();

		const u32 numMaterials = buff.Read<u32>();
		const i32 activeMaterialID = buff.Read<i32>();
		const u8 bMaterialNeedsUpdate = buff.Read<u8>();
		mesh.meshPrimitiveType = buff.Read<u32>();

		const u16 numSubMeshes = buff.Read<u16>();
		const u8 bInstancingEnabled = buff.Read<u8>();

		const Vector3 boundCenter = buff.Read<Vector3>();
		const f32 boundRadius = buff.Read<f32>();

		const u32 numDataStreams = buff.Read<u32>();
		for(int i = 0; i < numDataStreams; i++) {
			Mesh::DataStream datastream;
			datastream.refStream = buff.Read<i32>();
			datastream.bIsPerInstance = buff.Read<u8>();

			const u16 numSubMeshes = buff.Read<u16>();
			for(int si = 0; si < numSubMeshes; si++) {
				datastream.subMeshToRegionMapList.push_back(buff.Read<u16>());
			}

			const u32 numComponents = buff.Read<u32>();
			for(int ci = 0; ci < numComponents; ci++) {
				Mesh::DataStream::Component comp;
				const u32 nameStrID = buff.Read<u32>();
				comp.name = (nameStrID != 0xFFFFFFFF) ? stringList[nameStrID] : StringSlice{"_", 1};
				comp.index = buff.Read<u32>();

				datastream.componentList.push_back(comp);
			}

			mesh.dataStreamList.push_back(datastream);
		}

		const u32 numModifiers = buff.Read<u32>();
		eastl::fixed_vector<i32,128,true> refModifierList;
		for(int i = 0; i < numModifiers; i++) {
			refModifierList.push_back(buff.Read<i32>());
		}

		LOG("	name = '%.*s'", name.len, name.at);
		LOG("	extraDataList(%d) = [", numExtraDataList);
		foreach(it, refExtraDataList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	refController = %d", refController);
		LOG("	flags = %#x", flags);
		LOG("	translation = (%g, %g, %g)", translation.x, translation.y, translation.z);
		LOG("	rotation = (%.2f, %.2f, %.2f", rotation.data[0], rotation.data[1], rotation.data[2]);
		LOG("	            %.2f, %.2f, %.2f", rotation.data[3], rotation.data[4], rotation.data[5]);
		LOG("	            %.2f, %.2f, %.2f)", rotation.data[6], rotation.data[7], rotation.data[8]);
		LOG("	scale = %g", scale);
		LOG("	properties(%d) = [", numProperties);
		foreach(it, refPropertyList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	refCollisionObject = %d", refCollisionObject);
		LOG("");
		LOG("	numMaterials = %d", numMaterials);
		LOG("	activeMaterialID = %d", activeMaterialID);
		LOG("	bMaterialNeedsUpdate = %d", bMaterialNeedsUpdate);
		LOG("	meshPrimitiveType = %d", mesh.meshPrimitiveType);
		LOG("	numSubMeshes = %d", numSubMeshes);
		LOG("	bInstancingEnabled = %d", bInstancingEnabled);
		LOG("	dataStreams(%d) = [", numDataStreams);
		foreach(it, mesh.dataStreamList) {
			LOG("		{");
			LOG("			refStream = %d", it->refStream);
			LOG("			bIsPerInstance = %d", it->bIsPerInstance);
			LOG("			subMeshToRegionMapList(%zd) = [", it->subMeshToRegionMapList.size());
			foreach(sit, it->subMeshToRegionMapList) {
				LOG("				%d,", *sit);
			}
			LOG("			]");
			LOG("			components(%zd) = [", it->componentList.size());
			foreach(comp, it->componentList) {
				LOG("				(name='%.*s', index=%u),", comp->name.len, comp->name.at, comp->index);
			}
			LOG("			]");
			LOG("		},");
		}
		LOG("	]");
		LOG("	bound = {");
		LOG("		center = (%g, %g, %g)", boundCenter.x, boundCenter.y, boundCenter.z);
		LOG("		radius = %g", boundRadius);
		LOG("	}");
		return true;
	}

	bool ReadBlock_NiDataStream(ConstBuffer buff, i32 blockID)
	{
		const u32 numBytes = buff.Read<u32>();
		const u32 cloningBehaviour = buff.Read<u32>();

		struct Region
		{
			u32 startIndex;
			u32 numIndices;
		};

		const u32 numRegions = buff.Read<u32>();
		eastl::fixed_vector<Region,128,true> regionList;
		for(int i = 0; i < numRegions; i++) {
			const u32 startIndex = buff.Read<u32>();
			const u32 numIndices = buff.Read<u32>();
			regionList.push_back({ startIndex, numIndices });
		}

		const u32 numComponents = buff.Read<u32>();
		eastl::fixed_vector<u32,128,true> componentList;
		for(int i = 0; i < numComponents; i++) {
			const u32 format = buff.Read<u32>();
			componentList.push_back(format);
		}

		const void* data = buff.ReadRaw(numBytes);
		const u8 bStreamable = buff.Read<u8>();

		LOG("	numBytes = %d", numBytes);
		LOG("	cloningBehaviour = %d", cloningBehaviour);
		LOG("	regions(%d) = [", numRegions);
		foreach(it, regionList) {
			LOG("		(startIndex=%d numIndices=%u),", it->startIndex, it->numIndices);
		}
		LOG("	]");
		LOG("	components(%d) = [", numComponents);
		foreach(it, componentList) {
			LOG("		%d,", *it);
		}
		LOG("	]");
		LOG("	bStreamable = %d", bStreamable);

		ASSERT(regionList.size() == 1);
		ASSERT(regionList[0].startIndex == 0);
		dataStreamMap[blockID] = DataStream{ data, numBytes, regionList[0].numIndices };
		return true;
	}

	bool ReadBlock(ConstBuffer& buff, i32 blockID, const u16 blockType, const StringSlice& blockTypeStr)
	{
		bool r = true;

		LOG("Block (id=%d size=%d at=%d): {", blockID, blockSizeList[blockID], buff.GetCursorPos());
		LOG("	type = '%.*s' (%u)", blockTypeStr.len, blockTypeStr.at, blockType);

		void* at = buff.ReadRaw(blockSizeList[blockID]);
		ConstBuffer block(at, blockSizeList[blockID]);

		if(blockTypeStr.Equals("NiNode")) {
			r = ReadBlock_NiNode(block, blockID);
		}
		else if(blockTypeStr.Equals("NiStringExtraData")) {
			r = ReadBlock_NiStringExtraData(block, blockID);
		}
		else if(blockTypeStr.Equals("NiMesh")) {
			r = ReadBlock_NiMesh(block, blockID);
		}
		else if(blockTypeStr.StartsWith("NiDataStream")) {
			r = ReadBlock_NiDataStream(block, blockID);
		}
		else {
			LOG("	[Unknown block -- skipping]");
		}

		LOG("}");
		return r;
	}

	bool ReadFile()
	{
		ConstBuffer buff(fileData, fileSize);

		// header string
		FixedStr<80> headerStr(buff.Read<char[38]>(), 38);
		LOG("header = '%s'", headerStr.data());

		char headerEnd = buff.Read<char>();
		if(headerEnd != '\n') {
			return false;
		}

		const u32 fileVersion = buff.Read<u32>();
		LOG("fileVersion = %#x", fileVersion);

		const u8 littleEndian = buff.Read<u8>();
		if(littleEndian != 1) {
			LOG("ERROR: file is big endian");
			return false;
		}

		const u32 userVersion = buff.Read<u32>();
		const u32 numBlocks = buff.Read<u32>();
		LOG("numBlocks = %u", numBlocks);

		const u16 numBlockTypes = buff.Read<u16>();
		eastl::fixed_vector<StringSlice,1024,true> blockTypeStringList;

		//LOG("Block Type String(%d): {", numBlockTypes);
		for(int i = 0; i < numBlockTypes; i++) {
			const u32 len = buff.Read<u32>();
			const char* str = (char*)buff.ReadRaw(len);
			blockTypeStringList.push_back({str, len});
			//LOG("'%.*s'", len, str);
		}
		//LOG("}");

		//LOG("Block Type Index: {");
		eastl::fixed_vector<u16,1024,true> blockTypeIndexList;
		for(int i = 0; i < numBlocks; i++) {
			const u16 blockTypeIndex = buff.Read<u16>();
			blockTypeIndexList.push_back(blockTypeIndex);
			//LOG("%d", blockTypeIndex);
		}
		//LOG("}");

		//LOG("Block Size: {");
		for(int i = 0; i < numBlocks; i++) {
			const u32 blockSize = buff.Read<u32>();
			blockSizeList.push_back(blockSize);
			//LOG("%u", blockSize);
		}
		//LOG("}");

		const u32 numStrings = buff.Read<u32>();
		const u32 maxStringLength = buff.Read<u32>();
		//LOG("Strings(count=%d maxLen=%d): {", numStrings, maxStringLength);
		for(int i = 0; i < numStrings; i++) {
			const u32 len = buff.Read<u32>();
			const char* str = (char*)buff.ReadRaw(len);
			stringList.push_back({str, len});
			//LOG("'%.*s'", len, str);
		}
		//LOG("}");

		const u32 numGroups = buff.Read<u32>();
		LOG("numGroups = %u", numGroups);

		for(int i = 0; i < numBlocks; i++) {
			const u16 blockTypeIndex = blockTypeIndexList[i];
			const StringSlice& blockTypeStr = (blockTypeIndex < numBlockTypes) ? blockTypeStringList[blockTypeIndex] : StringSlice{"_", 1};

			bool r = ReadBlock(buff, i, blockTypeIndex, blockTypeStr);
			if(!r) {
				LOG("ERROR: failed to read block at <%d>", buff.GetCursorPos());
				return false;
			}
		}

		LOG("remainingSize = %d", buff.RemainingDataSize());

		return true;
	}
};

bool ExtractCollisionMesh(const CollisionNifReader& nif, const char* outPath)
{
	eastl::fixed_string<char,8192,true> out;

	if(nif.meshList.empty()) {
		LOG("ERROR: no mesh found");
		return false;
	}

	static PxDefaultErrorCallback gDefaultErrorCallback;
	static PxDefaultAllocator gDefaultAllocatorCallback;
	PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
	if(!foundation) {
		LOG("ERROR: PxCreateFoundation failed");
		return false;
	}

	PxCooking* cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(PxTolerancesScale{}));
	if(!foundation) {
		LOG("ERROR: PxCreateCooking failed");
		return false;
	}

	// mesh file
	GrowableBuffer outMesh(1024*1024);
	outMesh.Append("MESH", 4);
	const u16 meshVersion = 2;
	outMesh.Append(&meshVersion, sizeof(meshVersion));
	const u16 meshCount = nif.meshList.size();
	outMesh.Append(&meshCount, sizeof(meshCount));

	GrowableBuffer outCooked(1024*1024);
	outCooked.Append("PHYX", 4);
	const u16 phyxVersion = 1;
	outCooked.Append(&phyxVersion, sizeof(phyxVersion));
	outCooked.Append(&meshCount, sizeof(meshCount));

	int vertOffset = 1;

	foreach_const(it, nif.meshList) {
		const CollisionNifReader::Mesh& mesh = *it;

		if(mesh.meshPrimitiveType != 0) {
			LOG("ERROR: mesh primitive type not handled");
			return false;
		}
		if(mesh.dataStreamList.size() < 2) {
			LOG("ERROR: mesh dataStream count < 2");
			return false;
		}

		const CollisionNifReader::Mesh::DataStream& streamIndex = mesh.dataStreamList[0];
		const CollisionNifReader::Mesh::DataStream& streamVertices = mesh.dataStreamList[1];

		if(streamIndex.componentList.empty()) {
			LOG("ERROR: streamIndex has no components");
			return false;
		}

		// VERTICES
		if(streamVertices.componentList.size() < 3) {
			LOG("ERROR: streamVertices component count < 3");
			return false;
		}

		const i32 refVerticesBlock = streamVertices.refStream;
		if(refVerticesBlock == -1) {
			LOG("ERROR: refVerticesBlock is null");
			return false;
		}

		const CollisionNifReader::Mesh::DataStream::Component& compVertPosition = streamVertices.componentList[0];
		const CollisionNifReader::Mesh::DataStream::Component& compVertNormal = streamVertices.componentList[1];
		const CollisionNifReader::Mesh::DataStream::Component& compVertColor = streamVertices.componentList[2];
		if(!compVertPosition.name.Equals("POSITION")) {
			LOG("ERROR: compVertPosition is invalid");
			return false;
		}
		if(!compVertNormal.name.Equals("NORMAL")) {
			LOG("ERROR: compVertNormal is invalid");
			return false;
		}
		if(!compVertColor.name.Equals("COLOR")) {
			LOG("ERROR: compVertColor is invalid");
			return false;
		}

		auto found = nif.dataStreamMap.find(refVerticesBlock);
		if(found == nif.dataStreamMap.end()) {
			LOG("ERROR: verticesBlock not found");
			return false;
		}

		const CollisionNifReader::DataStream& verticesBlock = found->second;

		PUSH_PACKED;
		struct Vertex
		{
			Vector3 pos;
			Vector3 normal;
			u32 color;
		};
		POP_PACKED;
		STATIC_ASSERT(sizeof(Vertex) == 28);

		ASSERT(verticesBlock.size == sizeof(Vertex) * verticesBlock.numIndices);


		// INDICES
		const i32 refIndexBlock = streamIndex.refStream;
		if(refIndexBlock == -1) {
			LOG("ERROR: refIndexBlock is null");
			return false;
		}

		const CollisionNifReader::Mesh::DataStream::Component& compIndex = streamIndex.componentList[0];
		if(!compIndex.name.Equals("INDEX")) {
			LOG("ERROR: compIndex is invalid");
			return false;
		}

		found = nif.dataStreamMap.find(refIndexBlock);
		if(found == nif.dataStreamMap.end()) {
			LOG("ERROR: indexBlock not found");
			return false;
		}

		const CollisionNifReader::DataStream& indexBlock = found->second;

		ASSERT(indexBlock.size == sizeof(u16) * indexBlock.numIndices);
		ASSERT((indexBlock.numIndices/3) * 3 == indexBlock.numIndices);

		out.append(FMT("o %s\n", mesh.name.data()));

		// obj file
		const Vertex* vertices = (Vertex*)verticesBlock.at;
		for(int i = 0; i < verticesBlock.numIndices; i++) {
			const Vertex& v = vertices[i];
			out.append(FMT("v %g %g %g\n", v.pos.x, v.pos.z, v.pos.y));
			out.append(FMT("vn %g %g %g\n", -v.normal.x, -v.normal.z, -v.normal.y));
		}

		const u16* indices = (u16*)indexBlock.at;
		for(int i = 0; i < indexBlock.numIndices; i += 3) {
			out.append(FMT("f %d//%d %d//%d %d//%d\n",
						indices[i]+vertOffset, indices[i]+vertOffset, indices[i+1]+vertOffset,
						indices[i+1]+vertOffset, indices[i+2]+vertOffset, indices[i+2]+vertOffset));
		}
		vertOffset += verticesBlock.numIndices;

		out.append("\n");

		// mesh file

		// name
		const i32 nameLen = mesh.name.size();
		outMesh.Append(&nameLen, sizeof(nameLen));
		outMesh.Append(mesh.name.data(), nameLen);

		// vertexCount
		outMesh.Append(&verticesBlock.numIndices, sizeof(verticesBlock.numIndices));
		// indexCount
		outMesh.Append(&indexBlock.numIndices, sizeof(indexBlock.numIndices));

		for(int i = 0; i < verticesBlock.numIndices; i++) {
			const Vertex& v = vertices[i];
			Vector3 p = { v.pos.x, v.pos.y, v.pos.z };
			outMesh.Append(&p, sizeof(p));
			Vector3 n = { v.normal.x, v.normal.y, v.normal.z };
			outMesh.Append(&n, sizeof(n));
		}

		// make it so back is culled
		for(int i = 0; i < indexBlock.numIndices; i += 3) {
			outMesh.Append(&indices[i], sizeof(u16));
			outMesh.Append(&indices[i+2], sizeof(u16));
			outMesh.Append(&indices[i+1], sizeof(u16));
		}

		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = verticesBlock.numIndices;
		meshDesc.points.stride = sizeof(Vertex);
		meshDesc.points.data = vertices;

		meshDesc.triangles.count = indexBlock.numIndices/3;
		meshDesc.triangles.stride = 3*sizeof(u16);
		meshDesc.triangles.data = indices;

		meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;

		PxDefaultMemoryOutputStream writeBuffer;
		PxTriangleMeshCookingResult::Enum result;
		bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
		if(!status) {
			LOG("ERROR: cookTriangleMesh failed");
			return false;
		}

		const u32 cookedMeshSize = writeBuffer.getSize();
		outCooked.Append(&cookedMeshSize, sizeof(cookedMeshSize));
		outCooked.Append(writeBuffer.getData(), cookedMeshSize);
	}

	bool r = fileSaveBuff(FMT("%s.obj", outPath), out.data(), out.size());
	if(!r) {
		LOG("ERROR: could not write '%s.obj'", outPath);
		return false;
	}

	r = fileSaveBuff(FMT("%s.msh", outPath), outMesh.data, outMesh.size);
	if(!r) {
		LOG("ERROR: could not write '%s.msh'", outPath);
		return false;
	}

	r = fileSaveBuff(FMT("%s.cooked", outPath), outCooked.data, outCooked.size);
	if(!r) {
		LOG("ERROR: could not write '%s.cooked'", outPath);
		return false;
	}

	return true;
}

struct ObjReader
{
	const u8* fileData;
	const i32 fileSize;

	struct Vertex
	{
		f32 x, y, z;
		f32 nx, ny, nz;
	};

	eastl::fixed_vector<Vertex, 4096> vertices;
	eastl::fixed_vector<u16, 4096> indices;

	bool ConsumeLine(ConstBuffer& buff, eastl::string_view* out)
	{
		if(!buff.CanRead(1)) return false;

		const char* str = &buff.Read<char>();

		for(int i = 0; i < 1024; i++) {
			if(!buff.CanRead(1)) return false;

			if(buff.Read<char>() == '\n') {
				*out = eastl::string_view(str, i);
				return true;
			}
		}

		return false;
	}

	bool ReadFile()
	{
		ConstBuffer buff(fileData, fileSize);

		eastl::string_view line;
		i32 lineNum = 0;
		i32 vertID = 0;
		while(ConsumeLine(buff, &line)) {
			// LOG("%.*s", (int)line.size(), line.data());

			switch(line[0]) {
				case 'v': {
					if(line[1] == 'n') {
						Vertex& v = vertices[vertID];
						if(sscanf(line.data(), "vn %f %f %f", &v.nx, &v.ny, &v.nz) == 3) {
							vertID++;
						}
						else {
							LOG("ERROR: failed to parse line %d '%.*s'", lineNum, (int)line.size(), line.data());
						}
					}
					else {
						Vertex v;
						if(sscanf(line.data(), "v %f %f %f", &v.x, &v.y, &v.z) == 3) {
							vertices.push_back(v);
						}
						else {
							LOG("ERROR: failed to parse line %d '%.*s'", lineNum, (int)line.size(), line.data());
						}
					}
				} break;

				case 'f': {
					i32 triangle[3];
					i32 dummy[3];
					if(sscanf(line.data(), "f %d//%d %d//%d %d//%d", &triangle[0], &dummy[0], &triangle[1], &dummy[1], &triangle[2], &dummy[2]) == 6) {
						indices.push_back(triangle[0]-1);
						indices.push_back(triangle[1]-1);
						indices.push_back(triangle[2]-1);
					}
					else {
						LOG("ERROR: failed to parse line %d '%.*s'", lineNum, (int)line.size(), line.data());
					}
				} break;
			}

			lineNum++;
		}

		return true;
	}
};

bool CookMesh(const ObjReader& obj, const char* outputFilePath)
{
	static PxDefaultErrorCallback gDefaultErrorCallback;
	static PxDefaultAllocator gDefaultAllocatorCallback;
	PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
	if(!foundation) {
		LOG("ERROR: PxCreateFoundation failed");
		return false;
	}

	PxCooking* cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(PxTolerancesScale{}));
	if(!foundation) {
		LOG("ERROR: PxCreateCooking failed");
		return false;
	}

	// physx file
	GrowableBuffer outCooked(1024*1024);
	outCooked.Append("PHYX", 4);
	const u16 phyxVersion = 1;
	outCooked.Append(&phyxVersion, sizeof(phyxVersion));
	const u16 meshCount = 1;
	outCooked.Append(&meshCount, sizeof(meshCount));

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = obj.vertices.size();
	meshDesc.points.stride = sizeof(ObjReader::Vertex);
	meshDesc.points.data = obj.vertices.data();

	meshDesc.triangles.count = obj.indices.size()/3;
	meshDesc.triangles.stride = 3*sizeof(u16);
	meshDesc.triangles.data = obj.indices.data();

	meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;

	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;
	bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
	if(!status) {
		LOG("ERROR: cookTriangleMesh failed");
		return false;
	}

	const u32 cookedMeshSize = writeBuffer.getSize();
	outCooked.Append(&cookedMeshSize, sizeof(cookedMeshSize));
	outCooked.Append(writeBuffer.getData(), cookedMeshSize);

	bool r = fileSaveBuff(FMT("%s.cooked", outputFilePath), outCooked.data, outCooked.size);
	if(!r) {
		LOG("ERROR: could not write '%s.cooked'", outputFilePath);
		return false;
	}

	// mesh
	GrowableBuffer outMesh(1024*1024);
	outMesh.Append("MESH", 4);
	const u16 meshVersion = 2;
	outMesh.Append(&meshVersion, sizeof(meshVersion));
	outMesh.Append(&meshCount, sizeof(meshCount));

	// name
	const char* name = "AnObjMesh";
	const i32 nameLen = strlen(name);
	outMesh.Append(&nameLen, sizeof(nameLen));
	outMesh.Append(name, nameLen);

	// vertexCount
	const i32 vertexCount = obj.vertices.size();
	outMesh.Append(&vertexCount, sizeof(vertexCount));
	// indexCount
	const i32 indexCount = obj.indices.size();
	outMesh.Append(&indexCount, sizeof(indexCount));

	for(int i = 0; i < vertexCount; i++) {
		const ObjReader::Vertex& v = obj.vertices[i];
		Vector3 p = { v.x, v.y, v.z };
		outMesh.Append(&p, sizeof(p));
		Vector3 n = { v.nx, v.ny, v.nz };
		outMesh.Append(&n, sizeof(n));
	}

	// make it so back is culled
	for(int i = 0; i < indexCount; i += 3) {
		outMesh.Append(&obj.indices[i], sizeof(u16));
		outMesh.Append(&obj.indices[i+2], sizeof(u16));
		outMesh.Append(&obj.indices[i+1], sizeof(u16));
	}

	r = fileSaveBuff(FMT("%s.msh", outputFilePath), outMesh.data, outMesh.size);
	if(!r) {
		LOG("ERROR: could not write '%s.msh'", outputFilePath);
		return false;
	}
	return true;
}

// WARNING: not safe
inline bool StrEndsWidth(const char* str, const char* end)
{
	const int strLen = strlen(str);
	const int endLen = strlen(end);

	for(int i = 0; i < endLen; i++) {
		if(str[strLen-endLen+i] != end[i]) return false;
	}

	return true;
}

int main(int argc, char** argv)
{
	LogInit("col.log");

	if(argc < 3) {
		LOG("Usage: col 'inputfile' 'output/path'");
		LOG("	Supports nif and obj");
		return 1;
	}

	const char* inputFilename = argv[1];
	const char* outputFilePath = argv[2];

	i32 fileSize;
	const u8* fileData = fileOpenAndReadAll(inputFilename, &fileSize);
	if(!fileData) {
		LOG("ERROR: failed to open '%s'", inputFilename);
		return 1;
	}

	LOG("'%s'", inputFilename);
	LOG("fileSize = %d", fileSize);

	if(StrEndsWidth(inputFilename, ".nif")) {
		CollisionNifReader reader = { fileData, fileSize };
		if(!reader.ReadFile()) {
			LOG("ERROR: failed to process '%s'", inputFilename);
			return 1;
		}

		bool r = ExtractCollisionMesh(reader, outputFilePath);
		if(!r) {
			LOG("ERROR: failed to extract '%s'", inputFilename);
			return 1;
		}

		LOG("Success!");
	}
	else if(StrEndsWidth(inputFilename, ".obj")) {
		ObjReader reader = { fileData, fileSize };
		if(!reader.ReadFile()) {
			LOG("ERROR: failed to process '%s'", inputFilename);
			return 1;
		}

		bool r = CookMesh(reader, outputFilePath);
		if(!r) {
			LOG("ERROR: failed to cook mesh");
			return 1;
		}
		LOG("Success!");
	}
	else {
		LOG("ERROR: Unrecognized file format");
	}

	return 0;
}
