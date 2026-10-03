#include <common/base.h>
#include <common/collision_asset.h>
#include <EAStdC/EAString.h>
#include <EASTL/fixed_map.h>
#include <NxCooking.h>
#include <NxConvexMeshDesc.h>
#include <NxStream.h>
#include <NxTriangleMeshDesc.h>
#include <PhysXLoader.h>

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

struct CookingOutputStream : NxStream
{
	eastl::fixed_vector<u8,4096,true> data;

	NxU8 readByte() const override { ASSERT(false); return 0; }
	NxU16 readWord() const override { ASSERT(false); return 0; }
	NxU32 readDword() const override { ASSERT(false); return 0; }
	NxF32 readFloat() const override { ASSERT(false); return 0; }
	NxF64 readDouble() const override { ASSERT(false); return 0; }
	void readBuffer(void*, NxU32) const override { ASSERT(false); }

	NxStream& storeByte(NxU8 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeWord(NxU16 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeDword(NxU32 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeFloat(NxF32 value) override { return storeBuffer(&value, sizeof(value)); }
	NxStream& storeDouble(NxF64 value) override { return storeBuffer(&value, sizeof(value)); }

	NxStream& storeBuffer(const void* buffer, NxU32 size) override
	{
		const u8* bytes = static_cast<const u8*>(buffer);
		data.insert(data.end(), bytes, bytes + size);
		return *this;
	}
};

static void CopyAssetName(char* dst, u32 dstSize, const char* src, u32 srcLen)
{
	ASSERT(srcLen < dstSize);
	memset(dst, 0, dstSize);
	memcpy(dst, src, srcLen);
}

static void CopyTransform(CollisionAsset::Transform* dst, const Matrix3x3& rot, const Vector3& tr)
{
	memcpy(dst->rotation, rot.data, sizeof(dst->rotation));
	dst->translation[0] = tr.x;
	dst->translation[1] = tr.y;
	dst->translation[2] = tr.z;
}

static void FillIdentityTransform(CollisionAsset::Transform* dst)
{
	memset(dst, 0, sizeof(*dst));
	dst->rotation[0] = 1.0f;
	dst->rotation[4] = 1.0f;
	dst->rotation[8] = 1.0f;
}

static void FillSourceMaterial(CollisionAsset::Material* dst)
{
	memset(dst, 0, sizeof(*dst));
	dst->index = 1;
	dst->dynamicFriction = 0.3f;
	dst->staticFriction = 0.3f;
	dst->anisotropy[0] = 1.0f;
}

struct CookedPhysXMesh
{
	eastl::fixed_string<char,64,false> name;
	eastl::fixed_vector<u8,4096,true> data;
};

static bool WriteCookedNiMeshAsset(GrowableBuffer& outCooked, const eastl::fixed_vector<CookedPhysXMesh,8,true>& meshes, u32 shapeType)
{
	ASSERT(!meshes.empty());
	CollisionAsset::Header header;
	memset(&header, 0, sizeof(header));
	header.magic = 0x58594850;
	header.version = CollisionAsset::VERSION;
	header.meshCount = (u16)meshes.size();
	header.materialCount = 1;
	header.actorCount = (u16)meshes.size();
	outCooked.Append(&header, sizeof(header));

	CollisionAsset::Material material;
	FillSourceMaterial(&material);
	outCooked.Append(&material, sizeof(material));

	foreach_const(it, meshes) {
		ASSERT(!it->data.empty());
		const i32 nameLen = it->name.size();
		outCooked.Append(&nameLen, sizeof(nameLen));
		outCooked.Append(it->name.data(), nameLen);
		const u32 cookedSize = (u32)it->data.size();
		outCooked.Append(&cookedSize, sizeof(cookedSize));
		outCooked.Append(it->data.data(), cookedSize);
	}

	for(u16 i = 0; i < meshes.size(); i++) {
		CollisionAsset::Actor actor;
		memset(&actor, 0, sizeof(actor));
		CopyAssetName(actor.name, sizeof(actor.name), meshes[i].name.data(), meshes[i].name.size());
		FillIdentityTransform(&actor.globalPose);
		actor.density = 1.0f;
		actor.contactReportFlags = 1;
		actor.shapeCount = 1;
		outCooked.Append(&actor, sizeof(actor));

		CollisionAsset::Shape shape;
		memset(&shape, 0, sizeof(shape));
		CopyAssetName(shape.name, sizeof(shape.name), meshes[i].name.data(), meshes[i].name.size());
		FillIdentityTransform(&shape.localPose);
		shape.type = shapeType;
		shape.flags = 0x120008;
		shape.materialIndex = 1;
		shape.density = 1.0f;
		shape.mass = -1.0f;
		shape.skinWidth = 0.05f;
		shape.meshIndex = i;
		outCooked.Append(&shape, sizeof(shape));
	}
	return true;
}



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

		i32 parentBlockID;
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
		i32 blockID;
		eastl::fixed_string<char,64,false> name;
	};

	struct PhysXMesh
	{
		i32 blockID;
		eastl::fixed_string<char,64,false> name;
		const void* cooked;
		u32 cookedSize;
	};

	struct PhysXMaterial
	{
		CollisionAsset::Material rec;
	};

	struct PhysXBody
	{
		i32 blockID;
		CollisionAsset::Body rec;
	};

	struct PhysXShape
	{
		i32 blockID;
		i32 meshBlockID;
		CollisionAsset::Shape rec;
	};

	struct PhysXActor
	{
		i32 blockID;
		i32 bodyBlockID;
		eastl::fixed_vector<i32,8,true> shapeBlockIDs;
		CollisionAsset::Actor rec;
	};

	eastl::fixed_vector<StringSlice,1024,true> stringList;
	eastl::fixed_vector<u32,1024,true> blockSizeList;
	eastl::fixed_map<i32,DataStream,16,true> dataStreamMap;

	eastl::fixed_vector<Mesh,8,true> meshList;
	Node currentNode;

	eastl::fixed_vector<i32,8,true> collisionNodeList;
	eastl::fixed_vector<PhysXMesh,16,true> physxMeshList;
	eastl::fixed_vector<PhysXMaterial,8,true> physxMaterialList;
	eastl::fixed_vector<PhysXBody,8,true> physxBodyList;
	eastl::fixed_vector<PhysXShape,16,true> physxShapeList;
	eastl::fixed_vector<PhysXActor,16,true> physxActorList;

	bool ReadBlock_NiNode(ConstBuffer buff, i32 blockID)
	{
		const i32 stringIndex = buff.Read<i32>();
		const StringSlice& str = (stringIndex != -1) ? stringList[stringIndex] : StringSlice{"_", 1};
		currentNode.name.assign(str.at, str.len);
		currentNode.blockID = blockID;

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
		DBG("Mesh @%lld", buff.data - fileData);

		Mesh& mesh = meshList.push_back();
		mesh.parentBlockID = currentNode.blockID;
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
		for(int i = 0; i < numMaterials; i++) {
			const u32 matNameStrID = buff.Read<u32>();
			const i32 matExtraData = buff.Read<i32>();
		}

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

		LOG("	parentNodeID = %d", mesh.parentBlockID);
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

	bool ReadBlock_NiPhysXDynamicSrc(ConstBuffer buff, i32 blockID)
	{
		const u16 numNodes = buff.Read<u16>();
		LOG("	children(%u)=[", numNodes);
		for(int i = 0; i < numNodes; i++) {
			const i32 childBlockID = buff.Read<i32>();
			LOG("		%d,", childBlockID);
			collisionNodeList.push_back(childBlockID);
		}
		LOG("	]");
		return true;
	}

	bool ReadBlock_NiPhysXMeshDesc(ConstBuffer buff, i32 blockID)
	{
		const i32 nameStrID = buff.Read<i32>();
		const StringSlice& name = (nameStrID != -1) ? stringList[nameStrID] : StringSlice{"_", 1};

		const u32 cookedSize = buff.Read<u32>();
		const void* cooked = nullptr;
		if(cookedSize != 0) {
			cooked = buff.ReadRaw(cookedSize);
		}

		const u16 vertexMapSize = buff.Read<u16>();
		if(vertexMapSize != 0) {
			buff.ReadRaw(vertexMapSize * sizeof(u16));
		}

		const u32 meshFlags = buff.Read<u32>();
		const u32 pagingMode = buff.Read<u32>();
		const u8 flags = buff.Read<u8>();

		ASSERT(buff.RemainingDataSize() == 0);
		ASSERT(cookedSize != 0);
		ASSERT(cooked != nullptr);

		PhysXMesh& mesh = physxMeshList.push_back();
		mesh.blockID = blockID;
		mesh.name.assign(name.at, name.len);
		mesh.cooked = cooked;
		mesh.cookedSize = cookedSize;

		LOG("	name = '%.*s'", name.len, name.at);
		LOG("	cookedSize = %u", cookedSize);
		LOG("	vertexMapSize = %u", vertexMapSize);
		LOG("	meshFlags = %#x", meshFlags);
		LOG("	pagingMode = %u", pagingMode);
		LOG("	flags = %#x", flags);
		return true;
	}

	bool ReadBlock_NiPhysXShapeDesc(ConstBuffer buff, i32 blockID)
	{
		const u32 shapeType = buff.Read<u32>();
		const Matrix3x3 localRot = buff.Read<Matrix3x3>();
		const Vector3 localTr = buff.Read<Vector3>();
		const u32 flags = buff.Read<u32>();
		const u16 collisionGroup = buff.Read<u16>();
		const u16 materialIndex = buff.Read<u16>();
		const f32 density = buff.Read<f32>();
		const f32 mass = buff.Read<f32>();
		const f32 skinWidth = buff.Read<f32>();
		const i32 nameStrID = buff.Read<i32>();
		const StringSlice& name = (nameStrID != -1) ? stringList[nameStrID] : StringSlice{"_", 1};
		const u32 nonInteracting = buff.Read<u32>();
		const u32 bits0 = buff.Read<u32>();
		const u32 bits1 = buff.Read<u32>();
		const u32 bits2 = buff.Read<u32>();
		const u32 bits3 = buff.Read<u32>();

		i32 meshRef = -1;
		Vector3 dimensions = { 0, 0, 0 };
		if(shapeType == 5 || shapeType == 6) {
			meshRef = buff.Read<i32>();
		}
		else if(shapeType == 0) {
			buff.Read<f32>();
			buff.Read<Vector3>();
		}
		else if(shapeType == 1) {
			dimensions.x = buff.Read<f32>();
		}
		else if(shapeType == 2) {
			dimensions = buff.Read<Vector3>();
		}
		else if(shapeType == 3) {
			dimensions.x = buff.Read<f32>();
			dimensions.y = buff.Read<f32>();
			buff.Read<u32>();
		}
		else {
			ASSERT(false);
		}

		ASSERT(buff.RemainingDataSize() == 0);

		PhysXShape& shape = physxShapeList.push_back();
		memset(&shape.rec, 0, sizeof(shape.rec));
		shape.blockID = blockID;
		shape.meshBlockID = meshRef;
		CopyAssetName(shape.rec.name, sizeof(shape.rec.name), name.at, name.len);
		CopyTransform(&shape.rec.localPose, localRot, localTr);
		shape.rec.type = shapeType;
		shape.rec.flags = flags;
		shape.rec.group = collisionGroup;
		shape.rec.materialIndex = materialIndex;
		shape.rec.density = density;
		shape.rec.mass = mass;
		shape.rec.skinWidth = skinWidth;
		shape.rec.nonInteracting = nonInteracting;
		shape.rec.groupsMask[0] = bits0;
		shape.rec.groupsMask[1] = bits1;
		shape.rec.groupsMask[2] = bits2;
		shape.rec.groupsMask[3] = bits3;
		shape.rec.meshIndex = -1;
		shape.rec.dimensions[0] = dimensions.x;
		shape.rec.dimensions[1] = dimensions.y;
		shape.rec.dimensions[2] = dimensions.z;

		LOG("	name = '%.*s'", name.len, name.at);
		LOG("	shapeType = %u", shapeType);
		LOG("	localTranslation = (%g, %g, %g)", localTr.x, localTr.y, localTr.z);
		LOG("	rotation = (%.2f, %.2f, %.2f", localRot.data[0], localRot.data[1], localRot.data[2]);
		LOG("	            %.2f, %.2f, %.2f", localRot.data[3], localRot.data[4], localRot.data[5]);
		LOG("	            %.2f, %.2f, %.2f)", localRot.data[6], localRot.data[7], localRot.data[8]);
		LOG("	flags = %#x", flags);
		LOG("	collisionGroup = %u", collisionGroup);
		LOG("	materialIndex = %u", materialIndex);
		LOG("	density = %g", density);
		LOG("	mass = %g", mass);
		LOG("	skinWidth = %g", skinWidth);
		LOG("	nonInteracting = %#x", nonInteracting);
		LOG("	collisionBits = (%#x, %#x, %#x, %#x)", bits0, bits1, bits2, bits3);
		LOG("	meshRef = %d", meshRef);
		return true;
	}

	bool ReadBlock_NiPhysXActorDesc(ConstBuffer buff, i32 blockID)
	{
		const i32 nameStrID = buff.Read<i32>();
		const StringSlice& name = (nameStrID != -1) ? stringList[nameStrID] : StringSlice{"_", 1};
		const u32 numPoses = buff.Read<u32>();
		LOG("	name = '%.*s'", name.len, name.at);
		LOG("	poses(%u) = [", numPoses);
		ASSERT(numPoses >= 1);
		Matrix3x3 poseRot = {};
		Vector3 poseTr = {};
		for(u32 i = 0; i < numPoses; i++) {
			const Matrix3x3 rot = buff.Read<Matrix3x3>();
			const Vector3 tr = buff.Read<Vector3>();
			if(i == 0) {
				poseRot = rot;
				poseTr = tr;
			}
			LOG("		translation = (%g, %g, %g)", tr.x, tr.y, tr.z);
			LOG("		rotation = (%.2f, %.2f, %.2f", rot.data[0], rot.data[1], rot.data[2]);
			LOG("		            %.2f, %.2f, %.2f", rot.data[3], rot.data[4], rot.data[5]);
			LOG("		            %.2f, %.2f, %.2f)", rot.data[6], rot.data[7], rot.data[8]);
		}
		LOG("	]");
		const i32 bodyRef = buff.Read<i32>();
		const f32 density = buff.Read<f32>();
		const u32 actorFlags = buff.Read<u32>();
		const u16 actorGroup = buff.Read<u16>();
		const u16 dominanceGroup = buff.Read<u16>();
		const u32 contactReportFlags = buff.Read<u32>();
		const u16 forceFieldMaterial = buff.Read<u16>();
		const u32 numShapes = buff.Read<u32>();
		LOG("	bodyRef = %d", bodyRef);
		LOG("	density = %g", density);
		LOG("	actorFlags = %#x", actorFlags);
		LOG("	actorGroup = %u", actorGroup);
		LOG("	dominanceGroup = %u", dominanceGroup);
		LOG("	contactReportFlags = %#x", contactReportFlags);
		LOG("	forceFieldMaterial = %u", forceFieldMaterial);
		LOG("	shapes(%u) = [", numShapes);
		PhysXActor& actor = physxActorList.push_back();
		memset(&actor.rec, 0, sizeof(actor.rec));
		actor.blockID = blockID;
		actor.bodyBlockID = bodyRef;
		CopyAssetName(actor.rec.name, sizeof(actor.rec.name), name.at, name.len);
		CopyTransform(&actor.rec.globalPose, poseRot, poseTr);
		actor.rec.flags = actorFlags;
		actor.rec.group = actorGroup;
		actor.rec.dominanceGroup = dominanceGroup;
		actor.rec.contactReportFlags = contactReportFlags;
		actor.rec.forceFieldMaterial = forceFieldMaterial;
		actor.rec.density = density;
		actor.rec.hasBody = (bodyRef != -1) ? 1 : 0;
		actor.rec.shapeCount = numShapes;
		for(u32 i = 0; i < numShapes; i++) {
			const i32 shapeRef = buff.Read<i32>();
			LOG("		%d,", shapeRef);
			actor.shapeBlockIDs.push_back(shapeRef);
		}
		LOG("	]");
		const i32 parentRef = buff.Read<i32>();
		const i32 srcRef = buff.Read<i32>();
		const i32 destRef = buff.Read<i32>();
		LOG("	parentRef = %d", parentRef);
		LOG("	srcRef = %d", srcRef);
		LOG("	destRef = %d", destRef);
		ASSERT(buff.RemainingDataSize() == 0);
		return true;
	}

	bool ReadBlock_NiPhysXMaterialDesc(ConstBuffer buff, i32 blockID)
	{
		const u16 index = buff.Read<u16>();
		const u32 numStates = buff.Read<u32>();
		ASSERT(numStates >= 1);
		PhysXMaterial& material = physxMaterialList.push_back();
		memset(&material.rec, 0, sizeof(material.rec));
		material.rec.index = index;
		material.rec.dynamicFriction = buff.Read<f32>();
		material.rec.staticFriction = buff.Read<f32>();
		material.rec.restitution = buff.Read<f32>();
		material.rec.dynamicFrictionV = buff.Read<f32>();
		material.rec.staticFrictionV = buff.Read<f32>();
		material.rec.anisotropy[0] = buff.Read<f32>();
		material.rec.anisotropy[1] = buff.Read<f32>();
		material.rec.anisotropy[2] = buff.Read<f32>();
		material.rec.flags = buff.Read<u32>();
		material.rec.frictionCombine = buff.Read<u32>();
		material.rec.restitutionCombine = buff.Read<u32>();
		for(u32 i = 1; i < numStates; i++) {
			buff.ReadRaw(44);
		}
		ASSERT(buff.RemainingDataSize() == 0);
		LOG("	index = %u", index);
		LOG("	numStates = %u", numStates);
		return true;
	}

	bool ReadBlock_NiPhysXBodyDesc(ConstBuffer buff, i32 blockID)
	{
		const Matrix3x3 localRot = buff.Read<Matrix3x3>();
		const Vector3 localTr = buff.Read<Vector3>();
		PhysXBody& body = physxBodyList.push_back();
		memset(&body.rec, 0, sizeof(body.rec));
		body.blockID = blockID;
		CopyTransform(&body.rec.massLocalPose, localRot, localTr);
		body.rec.inertia[0] = buff.Read<f32>();
		body.rec.inertia[1] = buff.Read<f32>();
		body.rec.inertia[2] = buff.Read<f32>();
		body.rec.mass = buff.Read<f32>();
		const u32 numVels = buff.Read<u32>();
		if(numVels != 0) {
			body.rec.linearVelocity[0] = buff.Read<f32>();
			body.rec.linearVelocity[1] = buff.Read<f32>();
			body.rec.linearVelocity[2] = buff.Read<f32>();
			body.rec.angularVelocity[0] = buff.Read<f32>();
			body.rec.angularVelocity[1] = buff.Read<f32>();
			body.rec.angularVelocity[2] = buff.Read<f32>();
			if(numVels > 1) {
				buff.ReadRaw((numVels - 1) * 24);
			}
		}
		body.rec.wakeUpCounter = buff.Read<f32>();
		body.rec.linearDamping = buff.Read<f32>();
		body.rec.angularDamping = buff.Read<f32>();
		body.rec.maxAngularVelocity = buff.Read<f32>();
		body.rec.ccdMotionThreshold = buff.Read<f32>();
		body.rec.flags = buff.Read<u32>();
		body.rec.sleepLinearVelocity = buff.Read<f32>();
		body.rec.sleepAngularVelocity = buff.Read<f32>();
		body.rec.solverIterationCount = buff.Read<u32>();
		body.rec.sleepEnergyThreshold = buff.Read<f32>();
		body.rec.sleepDamping = buff.Read<f32>();
		body.rec.contactReportThreshold = buff.Read<f32>();
		ASSERT(buff.RemainingDataSize() == 0);
		LOG("	mass = %g", body.rec.mass);
		LOG("	flags = %#x", body.rec.flags);
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
		else if(blockTypeStr.Equals("NiPhysXDynamicSrc")) {
			r = ReadBlock_NiPhysXDynamicSrc(block, blockID);
		}
		else if(blockTypeStr.Equals("NiPhysXMeshDesc")) {
			r = ReadBlock_NiPhysXMeshDesc(block, blockID);
		}
		else if(blockTypeStr.Equals("NiPhysXShapeDesc")) {
			r = ReadBlock_NiPhysXShapeDesc(block, blockID);
		}
		else if(blockTypeStr.Equals("NiPhysXActorDesc")) {
			r = ReadBlock_NiPhysXActorDesc(block, blockID);
		}
		else if(blockTypeStr.Equals("NiPhysXMaterialDesc")) {
			r = ReadBlock_NiPhysXMaterialDesc(block, blockID);
		}
		else if(blockTypeStr.Equals("NiPhysXBodyDesc")) {
			r = ReadBlock_NiPhysXBodyDesc(block, blockID);
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
			const u16 blockTypeIndex = blockTypeIndexList[i] & 0x7FFF;
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

static i32 FindPhysXMeshIndex(const CollisionNifReader& nif, i32 blockID)
{
	if(blockID == -1) {
		return -1;
	}
	for(u16 i = 0; i < nif.physxMeshList.size(); i++) {
		if(nif.physxMeshList[i].blockID == blockID) {
			return i;
		}
	}
	ASSERT(false);
	return -1;
}

static const CollisionNifReader::PhysXShape* FindPhysXShape(const CollisionNifReader& nif, i32 blockID)
{
	foreach_const(it, nif.physxShapeList) {
		if(it->blockID == blockID) {
			return &*it;
		}
	}
	ASSERT(false);
	return nullptr;
}

static const CollisionNifReader::PhysXBody* FindPhysXBody(const CollisionNifReader& nif, i32 blockID)
{
	foreach_const(it, nif.physxBodyList) {
		if(it->blockID == blockID) {
			return &*it;
		}
	}
	ASSERT(false);
	return nullptr;
}

static bool WriteAuthoredCollisionAsset(const CollisionNifReader& nif, GrowableBuffer& outCooked)
{
	ASSERT(!nif.physxActorList.empty());
	CollisionAsset::Header header;
	memset(&header, 0, sizeof(header));
	header.magic = 0x58594850;
	header.version = CollisionAsset::VERSION;
	header.meshCount = (u16)nif.physxMeshList.size();
	header.materialCount = (u16)nif.physxMaterialList.size();
	header.actorCount = (u16)nif.physxActorList.size();
	outCooked.Append(&header, sizeof(header));

	foreach_const(it, nif.physxMaterialList) {
		outCooked.Append(&it->rec, sizeof(it->rec));
	}

	foreach_const(it, nif.physxMeshList) {
		ASSERT(it->cooked != nullptr);
		ASSERT(it->cookedSize != 0);
		const i32 nameLen = it->name.size();
		outCooked.Append(&nameLen, sizeof(nameLen));
		outCooked.Append(it->name.data(), nameLen);
		outCooked.Append(&it->cookedSize, sizeof(it->cookedSize));
		outCooked.Append(it->cooked, it->cookedSize);
	}

	foreach_const(actorIt, nif.physxActorList) {
		outCooked.Append(&actorIt->rec, sizeof(actorIt->rec));
		if(actorIt->rec.hasBody != 0) {
			const CollisionNifReader::PhysXBody* body = FindPhysXBody(nif, actorIt->bodyBlockID);
			outCooked.Append(&body->rec, sizeof(body->rec));
		}
		ASSERT(actorIt->shapeBlockIDs.size() == actorIt->rec.shapeCount);
		foreach_const(shapeId, actorIt->shapeBlockIDs) {
			const CollisionNifReader::PhysXShape* shape = FindPhysXShape(nif, *shapeId);
			CollisionAsset::Shape rec = shape->rec;
			rec.meshIndex = FindPhysXMeshIndex(nif, shape->meshBlockID);
			outCooked.Append(&rec, sizeof(rec));
		}
	}
	return true;
}

bool ExtractCollisionMesh(const CollisionNifReader& nif, const char* outPath, bool cookGeneric)
{
	eastl::fixed_string<char,8192,true> out;

	if(nif.meshList.empty()) {
		LOG("ERROR: no mesh found");
		return false;
	}

	NxCookingInterface* cooking = nullptr;
	if(nif.physxActorList.empty() && cookGeneric) {
		cooking = NxGetCookingLib(NX_PHYSICS_SDK_VERSION);
		if(!cooking || !cooking->NxInitCooking()) {
			LOG("ERROR: NxInitCooking failed");
			return false;
		}
	}
	defer(if(cooking) cooking->NxCloseCooking());

	eastl::fixed_vector<const CollisionNifReader::Mesh*,8> validMeshes;
	foreach_const(it, nif.meshList) {
		const CollisionNifReader::Mesh& mesh = *it;

		if(mesh.dataStreamList.size() > 2) {
			LOG("Mesh is probably not used for collision since it has a many data streams, skipping");
			continue;
		}

		const CollisionNifReader::Mesh::DataStream& streamVertices = mesh.dataStreamList[1];

		if(streamVertices.componentList.size() > 3) {
			LOG("Mesh is probably not used for collision since it has a lot of vertex components, skipping");
			continue;
		}

		validMeshes.push_back(&*it);
	}

	// mesh file
	GrowableBuffer outMesh(1024*1024);
	outMesh.Append("MESH", 4);
	const u16 meshVersion = 2;
	outMesh.Append(&meshVersion, sizeof(meshVersion));
	const u16 meshCount = validMeshes.size();
	outMesh.Append(&meshCount, sizeof(meshCount));

	GrowableBuffer outCooked(1024*1024);
	eastl::fixed_vector<CookedPhysXMesh,8,true> cookedMeshes;

	int vertOffset = 1;

	foreach_const(it, validMeshes) {
		const CollisionNifReader::Mesh& mesh = **it;

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
		if(streamVertices.componentList.size() < 2) {
			LOG("ERROR: streamVertices component count < 2");
			return false;
		}

		const i32 refVerticesBlock = streamVertices.refStream;
		if(refVerticesBlock == -1) {
			LOG("ERROR: refVerticesBlock is null");
			return false;
		}

		const CollisionNifReader::Mesh::DataStream::Component& compVertPosition = streamVertices.componentList[0];
		const CollisionNifReader::Mesh::DataStream::Component& compVertNormal = streamVertices.componentList[1];
		if(!compVertPosition.name.Equals("POSITION")) {
			LOG("ERROR: compVertPosition is invalid");
			return false;
		}
		if(!compVertNormal.name.Equals("NORMAL")) {
			LOG("ERROR: compVertNormal is invalid");
			return false;
		}

		auto found = nif.dataStreamMap.find(refVerticesBlock);
		if(found == nif.dataStreamMap.end()) {
			LOG("ERROR: verticesBlock not found");
			return false;
		}

		const CollisionNifReader::DataStream& verticesBlock = found->second;

		struct Vertex {
			Vector3 pos;
			Vector3 normal;
		};

		i32 vertexSize = 24; // pos, normal
		if(streamVertices.componentList.size() == 3) {
			vertexSize = 28; // pos, normal, colour
		}
		ASSERT(verticesBlock.size == vertexSize * verticesBlock.numIndices);


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
		const u8* vertices = (u8*)verticesBlock.at;
		for(int i = 0; i < verticesBlock.numIndices; i++) {
			const Vertex& v = *(Vertex*)vertices;
			vertices += vertexSize;

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

		vertices = (u8*)verticesBlock.at;
		for(int i = 0; i < verticesBlock.numIndices; i++) {
			const Vertex& v = *(Vertex*)vertices;
			vertices += vertexSize;
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

		if(cooking) {
			NxTriangleMeshDesc meshDesc;
			meshDesc.numVertices = verticesBlock.numIndices;
			meshDesc.pointStrideBytes = vertexSize;
			meshDesc.points = verticesBlock.at;

			meshDesc.numTriangles = indexBlock.numIndices/3;
			meshDesc.triangleStrideBytes = 3*sizeof(u16);
			meshDesc.triangles = indices;

			meshDesc.flags = NX_MF_16_BIT_INDICES;

			CookingOutputStream writeBuffer;
			bool status = cooking->NxCookTriangleMesh(meshDesc, writeBuffer);
			if(!status) {
				LOG("ERROR: cookTriangleMesh failed");
				return false;
			}

			CookedPhysXMesh& cooked = cookedMeshes.push_back();
			cooked.name = mesh.name;
			cooked.data.insert(cooked.data.end(), writeBuffer.data.begin(), writeBuffer.data.end());
		}
	}

	if(!nif.physxActorList.empty()) {
		if(!WriteAuthoredCollisionAsset(nif, outCooked)) {
			return false;
		}
	}
	else if(cookGeneric) {
		if(!WriteCookedNiMeshAsset(outCooked, cookedMeshes, 6)) {
			return false;
		}
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

	if(outCooked.size != 0) {
		r = fileSaveBuff(FMT("%s.physx_static", outPath), outCooked.data, outCooked.size);
		if(!r) {
			LOG("ERROR: could not write '%s.physx_static'", outPath);
			return false;
		}
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


bool CookConvexCylinder(NxCookingInterface* cooking, const char* outputFilePath)
{
	NxVec3 verts[64];
	const f32 add = 2*NxTwoPiF32/32;

	for(int i = 0; i < 32; i++) {
		f32 a = add * i;
		verts[i] = NxVec3(cosf(a) * 50.f, sinf(a) * 50.f, 0);
	}
	for(int i = 0; i < 32; i++) {
		f32 a = add * i;
		verts[i+32] = NxVec3(cosf(a) * 50.f, sinf(a) * 50.f, 100.0f);
	}

	NxConvexMeshDesc convexDesc;
	convexDesc.numVertices = 64;
	convexDesc.pointStrideBytes = sizeof(NxVec3);
	convexDesc.points = verts;
	convexDesc.flags = NX_CF_COMPUTE_CONVEX;

	CookingOutputStream writeBuffer;
	bool status = cooking->NxCookConvexMesh(convexDesc, writeBuffer);
	if(!status) {
		LOG("ERROR: cookConvexMesh failed");
		return false;
	}

	eastl::fixed_vector<CookedPhysXMesh,8,true> meshes;
	CookedPhysXMesh& cooked = meshes.push_back();
	cooked.name.assign("cylinder", 8);
	cooked.data.insert(cooked.data.end(), writeBuffer.data.begin(), writeBuffer.data.end());
	GrowableBuffer outCooked(1024*1024);
	if(!WriteCookedNiMeshAsset(outCooked, meshes, 5)) {
		return false;
	}

	bool r = fileSaveBuff(FMT("%s.physx_dynamic", outputFilePath), outCooked.data, outCooked.size);
	if(!r) {
		LOG("ERROR: could not write '%s.physx_dynamic'", outputFilePath);
		return false;
	}

	return true;
}

bool CookMesh(const ObjReader& obj, const char* outputFilePath)
{
	NxCookingInterface* cooking = NxGetCookingLib(NX_PHYSICS_SDK_VERSION);
	if(!cooking || !cooking->NxInitCooking()) {
		LOG("ERROR: NxInitCooking failed");
		return false;
	}
	defer(cooking->NxCloseCooking());

	const char* name = "AnObjMesh";
	const i32 nameLen = strlen(name);
	const u16 meshCount = 1;

	ObjReader::Vertex* fixedVertices = (ObjReader::Vertex*)memAlloc(sizeof(ObjReader::Vertex) * obj.vertices.size());
	defer(memFree(fixedVertices));
	u16* fixedIndices = (u16*)memAlloc(sizeof(u16) * obj.indices.size());
	defer(memFree(fixedIndices));

	for(int i = 0; i < obj.vertices.size(); i++) {
		fixedVertices[i] = obj.vertices[i];
		eastl::swap(fixedVertices[i].y, fixedVertices[i].z);
		eastl::swap(fixedVertices[i].ny, fixedVertices[i].nz);
	}

	memmove(fixedIndices, obj.indices.data(), sizeof(u16) * obj.indices.size());
	for(int i = 1; i < obj.indices.size(); i += 3) {
		eastl::swap(fixedIndices[i], fixedIndices[i+1]);
	}

	NxTriangleMeshDesc meshDesc;
	meshDesc.numVertices = obj.vertices.size();
	meshDesc.pointStrideBytes = sizeof(ObjReader::Vertex);
	meshDesc.points = fixedVertices;

	meshDesc.numTriangles = obj.indices.size()/3;
	meshDesc.triangleStrideBytes = 3*sizeof(u16);
	meshDesc.triangles = fixedIndices;

	meshDesc.flags = NX_MF_16_BIT_INDICES;

	CookingOutputStream writeBuffer;
	bool status = cooking->NxCookTriangleMesh(meshDesc, writeBuffer);
	if(!status) {
		LOG("ERROR: cookTriangleMesh failed");
		return false;
	}

	eastl::fixed_vector<CookedPhysXMesh,8,true> meshes;
	CookedPhysXMesh& cooked = meshes.push_back();
	cooked.name.assign(name, nameLen);
	cooked.data.insert(cooked.data.end(), writeBuffer.data.begin(), writeBuffer.data.end());
	GrowableBuffer outCooked(1024*1024);
	if(!WriteCookedNiMeshAsset(outCooked, meshes, 6)) {
		return false;
	}

	bool r = fileSaveBuff(FMT("%s.physx_static", outputFilePath), outCooked.data, outCooked.size);
	if(!r) {
		LOG("ERROR: could not write '%s.physx_static'", outputFilePath);
		return false;
	}

	r = CookConvexCylinder(cooking, outputFilePath);
	if(!r) return false;

	// mesh
	GrowableBuffer outMesh(1024*1024);
	outMesh.Append("MESH", 4);
	const u16 meshVersion = 2;
	outMesh.Append(&meshVersion, sizeof(meshVersion));
	outMesh.Append(&meshCount, sizeof(meshCount));

	// name
	outMesh.Append(&nameLen, sizeof(nameLen));
	outMesh.Append(name, nameLen);

	// vertexCount
	const i32 vertexCount = obj.vertices.size();
	outMesh.Append(&vertexCount, sizeof(vertexCount));
	// indexCount
	const i32 indexCount = obj.indices.size();
	outMesh.Append(&indexCount, sizeof(indexCount));

	outMesh.Append(fixedVertices, vertexCount * sizeof(ObjReader::Vertex));
	outMesh.Append(obj.indices.data(), indexCount * sizeof(u16));

	r = fileSaveBuff(FMT("%s.msh", outputFilePath), outMesh.data, outMesh.size);
	if(!r) {
		LOG("ERROR: could not write '%s.msh'", outputFilePath);
		return false;
	}
	return true;
}

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
		LOG("Usage: col 'inputfile' 'output/path' [-cook]");
		LOG("	Supports nif and obj");
		return 1;
	}

	const char* inputFilename = argv[1];
	const char* outputFilePath = argv[2];
	bool cookGeneric = false;
	for(int i = 3; i < argc; i++) {
		if(StringEquals(argv[i], "-cook")) {
			cookGeneric = true;
		}
	}

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

		bool r = ExtractCollisionMesh(reader, outputFilePath, cookGeneric);
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
