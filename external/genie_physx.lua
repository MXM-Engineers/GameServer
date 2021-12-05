-- physx

local SRC_DIR = "physx-4.1"
physx_includedir = {
	path.getabsolute(SRC_DIR .. "/include/physx"),
	path.getabsolute(SRC_DIR .. "/include/pxshared"),
}

physx_libdir_debug = {
	path.getabsolute(SRC_DIR .. "/lib/debug"),
	path.getabsolute(SRC_DIR .. "/lib/checked")
}

physx_libdir_release = path.getabsolute(SRC_DIR .. "/lib/release")

physx_libs_win64 = {
	"PhysXExtensions_static_64",
	"PhysX_64",
	"PhysXPvdSDK_static_64",
	"PhysXCommon_64",
	"PhysXFoundation_64",
	"PhysXCooking_64",
}
