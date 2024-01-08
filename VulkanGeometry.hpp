#ifndef __VK_GEOMETRY_HPP__
#define __VK_GEOMETRY_HPP__

namespace VkApplication {

	std::mutex writeHashTableLock;

	struct thread_pool_frustrum_culling {
		std::atomic_bool done;
		threadsafe_queue<std::function<void()>> work_queue;
		std::vector<std::thread> threads;
		join_threads joiner;

		void worker_thread() {
			while (!done) {
				std::function<void()> task;
				if (work_queue.try_pop(task)) task();
				else std::this_thread::yield();
			}
		}

		thread_pool_frustrum_culling() : done(false), joiner(threads) {
			unsigned const thread_count = std::thread::hardware_concurrency();

			try {
				for (size_t i = 0; i < thread_count; ++i)
					threads.push_back(std::thread(&thread_pool_frustrum_culling::worker_thread, this));
			}
			catch (...) {
				done = true; throw;
			}
		}

		~thread_pool_frustrum_culling() { done = true; }

		template<class FT>
		void submit(FT f) {
			work_queue.push(std::function<void()>(f));
		}
	};

	glm::vec3 parseVec3(const std::string& str) {
		std::stringstream ss(str.substr(1, str.size() - 2)); // Remove the parentheses
		float x, y, z;
		char comma; // Used to skip the commas
		ss >> x >> comma >> y >> comma >> z;
		return glm::vec3(x, y, z);
	}

	void processMesh(aiMesh* mesh, const aiScene* scene) {
		// Process vertices, normals, texture coordinates, etc.
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			// Example: mesh->mVertices[i], mesh->mNormals[i], etc.
		}

		// Process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				// Example: face.mIndices[j]
			}
		}

		// Process bones
		for (unsigned int i = 0; i < mesh->mNumBones; i++) {
			aiBone* bone = mesh->mBones[i];
			// Process bone data
		}
	}

	void processNode(aiNode* node, const aiScene* scene) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			processMesh(mesh, scene);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}

	inline bool isOutsidePlane(const AABB& box, const Plane& plane) {
		glm::vec3 positiveVertex = box.min;
		glm::vec3 negativeVertex = box.max;

		if (plane.normal.x >= 0) {
			positiveVertex.x = box.max.x;
			negativeVertex.x = box.min.x;
		}
		if (plane.normal.y >= 0) {
			positiveVertex.y = box.max.y;
			negativeVertex.y = box.min.y;
		}
		if (plane.normal.z >= 0) {
			positiveVertex.z = box.max.z;
			negativeVertex.z = box.min.z;
		}

		// If the negative vertex is outside, the whole AABB is outside the plane
		if (glm::dot(plane.normal, negativeVertex) + plane.d > 0) {
			return true;
		}

		// Even if the positive vertex is outside, the AABB still might intersect the plane
		return false;
	}

	inline bool intersectsFrustum(const AABB& box, const Frustrum& frustum) {
		
		for (const Plane& plane : frustum.planes) {
			if (isOutsidePlane(box, plane)) {
				return false; // the box is outside of this plane
			}
		}
		return true; // the box is inside all planes
	}

	void MainVulkApplication::pruneGeo(const UniformBufferObject& _ubo, const QuadTreeNode * const _node,
		std::unordered_set<std::string >& objectsToRenderForFrame, const std::unordered_map<std::string, AABB >& objectAABB) {

		if (_node == NULL) return;

		glm::mat4 clipMatrix = _ubo.proj * _ubo.view;
		glm::vec4 planes[6];
		// Left
		planes[0] = clipMatrix[3] + clipMatrix[0];
		// Right
		planes[1] = clipMatrix[3] - clipMatrix[0];
		// Bottom
		planes[2] = clipMatrix[3] + clipMatrix[1];
		// Top
		planes[3] = clipMatrix[3] - clipMatrix[1];
		// Near
		planes[4] = clipMatrix[3] + clipMatrix[2];
		// Far
		planes[5] = clipMatrix[3] - clipMatrix[2];

		Frustrum frustum;
		// normalize
		for (int i = 0; i < 6; ++i) {
			planes[i] /= glm::length(glm::vec3(planes[i]));
			frustum.planes[i].normal = glm::vec3(planes[i]);
			frustum.planes[i].d = planes[i].w;
		}

		if (intersectsFrustum(_node->box, frustum)) {
			{
				std::lock_guard<std::mutex> lk(writeHashTableLock);
				if (_node->isLeaf == true) {
					for (auto& IDs : _node->objectIDs) objectsToRenderForFrame.insert(IDs);
				}
			}
			for (QuadTreeNode* child : _node->children) {
				FrustCullThreadPool->submit([&]() { pruneGeo(_ubo, child, objectsToRenderForFrame, objectAABB); });
			}
		}
	}

	void MainVulkApplication::loadModel() {

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        int numberOfPoints = 0;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, WORLD_PATH.c_str(), 0, true)) {
			throw std::runtime_error(warn + err);
		}

		//WORLD
		for (const auto& shape : shapes) {
			
			objectHash.insert({ shape.name, numberOfPoints });
			if (numberOfPoints == 0) std::cout << numberOfPoints << std::endl;
			for (int i = 0; i < shape.mesh.indices.size(); i += 3) {

				Vertex v1, v2, v3;

				v1.pos = { attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 2] };
				v2.pos = { attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 2] };
				v3.pos = { attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 2] };

				v1.color = { 1.0f, 1.0f, 1.0f };
				v2.color = { 1.0f, 1.0f, 1.0f };
				v3.color = { 1.0f, 1.0f, 1.0f };

				v1.vertexNormal = glm::vec3(attrib.normals[3 * shape.mesh.indices[i].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[i].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[i].normal_index + 2]);
				v2.vertexNormal = glm::vec3(attrib.normals[3 * shape.mesh.indices[i+1].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[i+1].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[i+1].normal_index + 2]);
				v3.vertexNormal = glm::vec3(attrib.normals[3 * shape.mesh.indices[i+2].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[i+2].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[i+2].normal_index + 2]);

				vertices.push_back(v1);
				vertices.push_back(v2);
				vertices.push_back(v3);

				indices.push_back(numberOfPoints++);
				indices.push_back(numberOfPoints++);
				indices.push_back(numberOfPoints++);
			}
		}
		numberOfPoints = 0;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, WORLD_PATH_LOWPOLY.c_str(), 0, true)) {
			throw std::runtime_error(warn + err);
		}

		//WORLD
		for (const auto& shape : shapes) {
			
			objectHash_lowpoly.insert({ shape.name, numberOfPoints });
			if (numberOfPoints == 0) std::cout << numberOfPoints << std::endl;
			for (int i = 0; i < shape.mesh.indices.size(); i += 3) {

				Vertex v1, v2, v3;

				v1.pos = { attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 2] };
				v2.pos = { attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 2] };
				v3.pos = { attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 2] };

				v1.color = { 1.0f, 1.0f, 1.0f };
				v2.color = { 1.0f, 1.0f, 1.0f };
				v3.color = { 1.0f, 1.0f, 1.0f };

				v1.vertexNormal = glm::vec3(attrib.normals[3 * shape.mesh.indices[i].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[i].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[i].normal_index + 2]);
				v2.vertexNormal = glm::vec3(attrib.normals[3 * shape.mesh.indices[i + 1].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[i + 1].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[i + 1].normal_index + 2]);
				v3.vertexNormal = glm::vec3(attrib.normals[3 * shape.mesh.indices[i + 2].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[i + 2].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[i + 2].normal_index + 2]);
				
				vertices_lowpoly.push_back(v1);
				vertices_lowpoly.push_back(v2);
				vertices_lowpoly.push_back(v3);

				indices_lowpoly.push_back(numberOfPoints++);
				indices_lowpoly.push_back(numberOfPoints++);
				indices_lowpoly.push_back(numberOfPoints++);
			}
		}

		std::ifstream World_AABB_File(WORLD_AABB_PATH);std::string line, cell;
		// Check if file is opened successfully
		if (!World_AABB_File.is_open()) {
			std::cerr << "Error opening file" << std::endl; exit(-1);
		}

		// skip first line
		std::getline(World_AABB_File, line);
		while (std::getline(World_AABB_File, line)) {
			std::istringstream linestream(line); std::string objectName;
			objectName.erase(std::remove(objectName.begin(), objectName.end(), '\"'), objectName.end());
			std::getline(linestream, objectName, ',');
			std::vector<glm::vec3> tempCorners;
			std::string cornerStr;
			std::vector<float> tempCooords;
			while (std::getline(linestream, cornerStr, ',')) {
				cornerStr.erase(std::remove(cornerStr.begin(), cornerStr.end(), '\"'), cornerStr.end());
				tempCooords.push_back(std::stof(cornerStr));
				if (tempCooords.size() == 3) {
					tempCorners.push_back(glm::vec3(tempCooords[0], tempCooords[1], tempCooords[2]));
					tempCooords.clear();
				}
			}
			AABB bbox(tempCorners[0], tempCorners[1], tempCorners[2], tempCorners[3], tempCorners[4], tempCorners[5], tempCorners[6], tempCorners[7] );
			if (objectAABB.find(objectName) != std::end(objectAABB)) {
				std::cerr << "Object ID already exists in AABB container." << std::endl;
				exit(-1);
			}
			objectAABB.insert({ objectName, bbox });
		}
		World_AABB_File.close();

		if (objectAABB.size() != objectHash.size()) {
			std::cerr << "Mismatched size for the number of objects in the scene and their respective bounding boxes." << std::endl;
			exit(-1);
		}

		for (auto& v : objectHash) {
			if (objectAABB.find(v.first) == end(objectAABB)) {
				std::cerr << "Error looking up objects name in AABB hash table vs object table." << std::endl;
				exit(-1);
			}
		}

		worldQuadTree = QuadTree(objectAABB);

		/*
		const aiScene* AVATAR = importer.ReadFile(AVATAR_PATH.c_str(),
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace);

		if (!AVATAR || AVATAR->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !AVATAR->mRootNode) {
			std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}

		processNode(AVATAR->mRootNode, AVATAR);
		*/
	}
}
#endif