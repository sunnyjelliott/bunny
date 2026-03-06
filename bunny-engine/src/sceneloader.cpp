#include "sceneloader.h"

bool SceneLoader::loadScene(const std::string& filepath, World& world,
                            const SceneLoadOptions& options) {
	SceneFormat format = options.format;
	if (format == SceneFormat::AUTO) {
		format = detectFormat(filepath);
	}

	switch (format) {
		case SceneFormat::USD:
			return loadUSD(filepath, world, options);
		case SceneFormat::OBJ:
			return loadOBJ(filepath, world, options);
		default:
			std::cerr << "Unsupported scene file format" << filepath
			          << std::endl;
			return false;
	}
}

SceneFormat SceneLoader::detectFormat(const std::string& filepath) {
	size_t extPos = filepath.find_last_of('.');

	// no extension, can't resolve
	if (extPos == std::string::npos) {
		return SceneFormat::AUTO;
	}

	std::string ext = filepath.substr(extPos + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(),
	               [](unsigned char c) { return std::tolower(c); });

	if (ext == "usd" || ext == "usda" || ext == "usdc") {
		return SceneFormat::USD;
	} else if (ext == "obj") {
		return SceneFormat::OBJ;
	}

	// no match, can't resolve
	return SceneFormat::AUTO;
}

bool SceneLoader::loadUSD(const std::string& filepath, World& world,
                          const SceneLoadOptions& options) {
	std::cout << "Loading USD scene: " << filepath << std::endl;

	std::filesystem::path absolutepath = std::filesystem::absolute(filepath);

	// Open USD stage
	UsdStageRefPtr stage = UsdStage::Open(absolutepath.generic_string());
	if (!stage) {
		std::cerr << "Failed to open USD stage: " << filepath << std::endl;
		return false;
	}
	std::cout << "Stage opened" << std::endl;

	// Get root prim
	UsdPrim rootPrim = stage->GetPseudoRoot();
	if (!rootPrim) {
		std::cerr << "USD stage has no root prim" << std::endl;
		return false;
	}
	std::cout << "Root prim valid" << std::endl;

	// Check if root has children
	auto children = rootPrim.GetChildren();
	if (children.empty()) {
		std::cout << "Root prim has no children" << std::endl;
		return true;  // Not an error, just empty
	}
	std::cout << "Root has children" << std::endl;

	// Traverse the scene hierarchy
	int entityCount = 0;
	for (const UsdPrim& child : children) {
		std::cout << "Processing child: " << child.GetPath().GetString()
		          << std::endl;
		Entity entity = traverseUsdPrim(child, world, options.parentEntity);
		if (entity != NULL_ENTITY) {
			entityCount++;
		}
	}

	std::cout << "Created " << entityCount << " entities from USD scene"
	          << std::endl;
	return true;
}

bool SceneLoader::loadOBJ(const std::string& filepath, World& world,
                          const SceneLoadOptions& options) {
	std::cout << "Loading OBJ: " << filepath << std::endl;

	std::cerr << "OBJ loading not yet implemented in SceneLoader" << std::endl;
	return false;
}

Entity SceneLoader::traverseUsdPrim(const UsdPrim& prim, World& world,
                                    Entity parent) {
	if (!prim.IsActive()) {
		return NULL_ENTITY;
	}

	std::cout << "Traversing prim: " << prim.GetPath().GetString()
	          << " (type: " << prim.GetTypeName() << ")" << std::endl;

	Entity entity = world.createEntity();

	Transform transform;
	extractUsdTransform(prim, transform);
	world.addComponent(entity, transform);

	if (parent != NULL_ENTITY) {
		world.setParent(entity, parent);
	}

	if (isUsdGeometry(prim)) {
		std::cout << "  -> Geometry prim detected" << std::endl;
		uint32_t meshID = createMeshFromUsdGeom(prim);

		MeshRenderer renderer;
		renderer.meshID = meshID;
		renderer.visible = true;

		world.addComponent(entity, renderer);
	}

	for (const UsdPrim& child : prim.GetChildren()) {
		traverseUsdPrim(child, world, entity);
	}

	return entity;
}

void SceneLoader::extractUsdTransform(const UsdPrim& prim,
                                      Transform& transform) {
	if (!prim.IsValid()) {
		std::cerr << "Invalid prim passed to extractUsdTransform" << std::endl;
		return;
	}

	// Check if this prim has transform ops
	UsdGeomXformable xformable(prim);
	if (!xformable) {
		return;  // Not a transformable prim
	}

	// Get local transformation matrix
	GfMatrix4d localMatrix;
	bool resetsXformStack;
	if (!xformable.GetLocalTransformation(&localMatrix, &resetsXformStack)) {
		std::cerr << "Failed to get local transformation for: "
		          << prim.GetPath().GetString() << std::endl;
		return;
	}

	GfMatrix4d r, u, p;
	GfVec3d s, t;
	localMatrix.Factor(&r, &s, &u, &t, &p);

	transform.position = glm::vec3(t[0], t[1], t[2]);
	transform.scale = glm::vec3(s[0], s[1], s[2]);

	GfMatrix4d rotationMatrix = localMatrix.RemoveScaleShear();
	rotationMatrix.SetTranslateOnly(GfVec3d(0, 0, 0));
	GfQuatd quat = rotationMatrix.ExtractRotationQuat();

	transform.rotation = glm::quat(static_cast<float>(quat.GetReal()),
	                               static_cast<float>(quat.GetImaginary()[0]),
	                               static_cast<float>(quat.GetImaginary()[1]),
	                               static_cast<float>(quat.GetImaginary()[2]));
}

bool SceneLoader::isUsdGeometry(const UsdPrim& prim) {
	return prim.IsA<UsdGeomMesh>() || prim.IsA<UsdGeomCube>() ||
	       prim.IsA<UsdGeomSphere>() || prim.IsA<UsdGeomCone>() ||
	       prim.IsA<UsdGeomCylinder>();
}

uint32_t SceneLoader::createMeshFromUsdGeom(const UsdPrim& prim) {
	// Handle explicit mesh data
	if (prim.IsA<UsdGeomMesh>()) {
		UsdGeomMesh mesh(prim);

		// Get vertex positions
		UsdAttribute pointsAttr = mesh.GetPointsAttr();
		VtArray<GfVec3f> points;
		pointsAttr.Get(&points);

		// Get face vertex indices
		UsdAttribute faceVertexIndicesAttr = mesh.GetFaceVertexIndicesAttr();
		VtArray<int> faceVertexIndices;
		faceVertexIndicesAttr.Get(&faceVertexIndices);

		// Get face vertex counts
		UsdAttribute faceVertexCountsAttr = mesh.GetFaceVertexCountsAttr();
		VtArray<int> faceVertexCounts;
		faceVertexCountsAttr.Get(&faceVertexCounts);

		std::cout << "  USD Mesh: " << points.size() << " points, "
		          << faceVertexCounts.size() << " faces" << std::endl;

		// TODO: Triangulate and upload mesh
		// For now, return 0 (cube) as placeholder
		return 0;
	}

	// Handle schema primitives - map to hardcoded meshes
	if (prim.IsA<UsdGeomCube>()) {
		std::cout << "  -> Mapping to hardcoded Cube mesh (ID: 0)" << std::endl;
		return 0;  // Cube mesh
	}

	if (prim.IsA<UsdGeomSphere>()) {
		std::cout << "  -> Mapping to hardcoded Sphere mesh (ID: 1)"
		          << std::endl;
		return 1;  // Sphere mesh
	}

	if (prim.IsA<UsdGeomCone>()) {
		std::cout << "  -> Mapping to hardcoded Cone/Pyramid mesh (ID: 1)"
		          << std::endl;
		return 2;  // Cone mesh
	}

	if (prim.IsA<UsdGeomCylinder>()) {
		std::cout << "  -> Cylinder not yet supported" << std::endl;
		return 0;  // Fallback to cube
	}

	std::cerr << "  -> Unknown geometry type: " << prim.GetTypeName()
	          << std::endl;
	return 0;
}
