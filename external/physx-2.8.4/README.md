# NVIDIA PhysX SDK 2.8.4

Packaged from the supplied `external/physx-2.8.4_install` distribution. Vendor headers and binaries are copied without modification.

- `include/physx/`: merged public include trees from Foundation, Physics, NxCharacter, Cooking and PhysXLoader (204 files).
- `lib/release/`: native Windows x64 loader, core, cooking and character-controller DLLs and supplied import libraries.
- `lib/checked/`: supplied Windows x64 CHECKED components and import libraries. Project Debug builds use CHECKED; this SDK distribution does not provide a separate Debug core.
- `lib/release/cudart64_30_9.dll`: mandatory hard import of both supplied core DLLs. It is copied for every configuration even though SDK hardware support is disabled and scenes use software simulation.

The CMake `physx` interface selects this package. Release executables and DLLs are deployed in `build/`; Debug executables retain `_dbg` names and are deployed in `build/checked/`. Run either configuration with `build/` as the working directory for configuration and game data.

The supplied CHECKED loader requests the unsuffixed `PhysXCore64.dll` and `PhysXCooking64.dll`. CMake therefore deploys the unchanged CHECKED core/cooking bytes under those requested names in the isolated Debug directory. Loader and controller DLLs retain their CHECKED names. This prevents Debug from silently using Release core/cooking or overwriting a Release deployment. The application uses the native legacy Nx API, not a PhysX 4 compatibility layer.

The supplied x86 DLLs differ byte-for-byte from the MxM client's x86 DLLs. Matching SDK version and the original legacy algorithms improves the basis for alignment but is not proof of identical builds or cross-architecture floating-point results. Runtime and request-replay results are recorded in the project's physics alignment documentation.

Vendor copyright notices are retained in every header. No standalone SDK license/EULA was found in the supplied installation; this package does not import or claim the PhysX 4 BSD license. Redistribution remains subject to the original SDK terms.
