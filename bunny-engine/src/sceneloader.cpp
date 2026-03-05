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
		// TODO: Create mesh from USD geometry
		// uint32_t meshID = createMeshFromUsdGeom(prim);
		// world.addComponent(entity, MeshRenderer{.meshID = meshID});
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
	std::cerr << "Mesh extraction not yet implemented" << std::endl;
	return 0;
}
