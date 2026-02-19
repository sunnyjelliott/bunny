#include "app.h"
#include "pch.h"

int main() {
	UsdStageRefPtr stage = UsdStage::CreateInMemory();
	if (stage) {
		std::cout << "USD smoke test passed - stage created successfully\n";

		// Create a simple prim to verify USD is fully functional
		UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/Root"));
		if (root) {
			std::cout << "USD can create prims\n";
		}
	} else {
		std::cerr << "USD smoke test FAILED - could not create stage\n";
	}

	Application app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}