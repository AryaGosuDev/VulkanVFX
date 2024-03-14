#ifndef __VK_GEOMETRY_HPP__
#define __VK_GEOMETRY_HPP__

#define EPS 1e-6

namespace VkApplication {

	std::mutex writeHashTableLock;

	struct Thread_pool_frustrum_culling {
		std::atomic_bool done;
		threadsafe_queue<std::function<void()>> work_queue;
		std::vector<std::thread> threads;
		join_threads joiner;

		std::mutex queue_mutex;
		std::condition_variable finish_condition;
		std::atomic<bool> start = false;
		std::atomic<int> tasks_in_progress = 0;

		void worker_thread() {
			while (!done) {
				std::function<void()> task;
				if (work_queue.try_pop(task)) {
					task();
					tasks_in_progress--;
					
				}
				else std::this_thread::yield();
				finish_condition.notify_all();
			}
		}

		Thread_pool_frustrum_culling() : done(false), joiner(threads) {
			unsigned thread_count = std::thread::hardware_concurrency();
			//thread_count = 1;
			try {
				for (size_t i = 0; i < thread_count; ++i)
					threads.push_back(std::thread(&Thread_pool_frustrum_culling::worker_thread, this));
			}
			catch (...) {
				done = true; throw("Error in thread_pool_frustrum_culling : Ending thread");
			}
		}

		~Thread_pool_frustrum_culling() { done = true; }

		template<class FT>
		void submit(FT f) {
			work_queue.push(std::function<void()>(f));
			tasks_in_progress++;
		}

		void join_all() {
			done = true;
			for (size_t i = 0; i < threads.size(); ++i) {
				if (threads[i].joinable()) {
					threads[i].join();
				}
			}
		}

		void waitUntilDone() {
			std::unique_lock<std::mutex> lock(queue_mutex);
			finish_condition.wait(lock, [this]() { return start && work_queue.empty() && tasks_in_progress == 0; });
			start = false;
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

	inline bool isAABBOutsidePlane(const AABB& box, const Plane& plane) {
		// Count vertices outside the plane
		int outsideVertices = 0;

		// List all eight vertices of the AABB
		std::vector<glm::vec3> vertices = {
			glm::vec3(box.min.x, box.min.y, box.min.z),
			glm::vec3(box.min.x, box.min.y, box.max.z),
			glm::vec3(box.min.x, box.max.y, box.min.z),
			glm::vec3(box.min.x, box.max.y, box.max.z),
			glm::vec3(box.max.x, box.min.y, box.min.z),
			glm::vec3(box.max.x, box.min.y, box.max.z),
			glm::vec3(box.max.x, box.max.y, box.min.z),
			glm::vec3(box.max.x, box.max.y, box.max.z)
		};

		// Check each vertex to see if it is outside the plane
		for (const auto& vertex : vertices) {
			double dotDistance = glm::dot(plane.normal, vertex);
			//if (glm::dot(plane.normal, vertex) + plane.d > 0.0f) {
			if (dotDistance + plane.d < -EPS) {
				outsideVertices++;
			}
		}

		// If all vertices are outside the plane, the AABB is outside this plane
		return outsideVertices == 8;
	}


	inline bool isAABBOutsideFrustum(const AABB& box, const Frustrum& frustum) {
		// Check if the AABB is outside any of the frustum planes
		for (const auto& plane : frustum.planes) {
			if (isAABBOutsidePlane(box, plane)) {
				// If the AABB is outside one plane, it's outside the frustum
				return true;
			}
		}

		// The AABB is not outside any of the planes, so it's at least partially inside the frustum
		return false;
	}

	inline bool intersectsFrustum(const AABB& box, const Frustrum& frustum) {
		if (isAABBOutsideFrustum(box, frustum)) return false;
		return true; // the box is inside all planes
	}

	void MainVulkApplication::pruneGeo(const glm::mat4 proj, const glm::mat4 view, std::shared_ptr< const QuadTreeNode* const> _node) {

		if ((*_node) == NULL) return;

		glm::mat4 tempProj = proj;
		glm::mat4 tempView = view;
		tempProj[1][1] *= -1.0f;

		glm::mat4 clipMatrix = tempProj * tempView;
		glm::vec4 planes[6];

		glm::vec4 rowX = glm::row(clipMatrix, 0);
		glm::vec4 rowY = glm::row(clipMatrix, 1);
		glm::vec4 rowZ = glm::row(clipMatrix, 2);
		glm::vec4 rowW = glm::row(clipMatrix, 3);

		// Left
		planes[0] = rowW - rowX;
		// Right
		planes[1] = rowW + rowX;
		// Bottom
		planes[2] = rowW + rowY;
		// Top
		planes[3] = rowW - rowY;
		// Near
		planes[4] = rowW + rowZ;
		// Far
		planes[5] = rowW - rowZ;

		Frustrum frustum;
		// normalize
		for (int i = 0; i < 6; ++i) {
			double primeLength = glm::length(glm::vec3(planes[i]));
			planes[i].x /= primeLength;
			planes[i].y /= primeLength;
			planes[i].z /= primeLength;
			frustum.planes[i].normal = glm::vec3(planes[i]);
			frustum.planes[i].d = planes[i].w / primeLength;
		}

		if (intersectsFrustum((*_node)->box, frustum)) {
			{
				if ((*_node)->isLeaf == true) {
					std::lock_guard<std::mutex> lk(writeHashTableLock);
					for (const auto& IDs : (*_node)->objectIDs) objectsToRenderForFrame.insert(IDs);
				}
				else {
					for (QuadTreeNode* child : (*_node)->children) {
						if (child != NULL) {
							auto childShared = std::make_shared< const QuadTreeNode* const>(child);
							FrustCullThreadPool->submit([=]() { pruneGeo(proj, view, childShared); });
						}
						
					}
				}
			}
			
		}
	}

	struct VertexIndex {
		int position;
		int normal;

		bool operator==(const VertexIndex& other) const {
			return position == other.position && normal == other.normal;
		}
	};

	// Define a hash functor for VertexIndex
	struct VertexIndexHash {
		size_t operator()(VertexIndex const& vertexIndex) const {
			size_t h1 = std::hash<int>()(vertexIndex.position);
			size_t h2 = std::hash<int>()(vertexIndex.normal);
			return h1 ^ (h2 << 1); // Combine the hash values
		}
	};

	void MainVulkApplication::loadModel() {

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        uint32_t numberOfPoints = 0;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, WORLD_PATH.c_str(), 0, true)) 
			throw std::runtime_error(warn + err);

		std::unordered_map<VertexIndex, uint32_t, VertexIndexHash,std::equal_to<VertexIndex> > vertexIndexToUniqueIndex;
		
		//WORLD
		/*
		for (const auto& shape : shapes) {
			
			objectHash.insert({ shape.name, numberOfPoints });
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
		*/

		// creates a new vertex only if the unique combination of vertex pos and normal are unique.
		// in vulkan, vertex buffers are input as a combination of vertex position and normals in one instance.
		// there are no seperate buffers for vertex, normals and tex coords

		for (const auto& shape : shapes) {
			uint32_t startIndices = indices.size();
			objectHash.insert({ shape.name, std::pair<uint32_t,uint32_t>(startIndices,0) });
			for (const auto& index : shape.mesh.indices) {
				VertexIndex vertexIndex{ index.vertex_index, index.normal_index };
				
				if (vertexIndexToUniqueIndex.count(vertexIndex) == 0) {
					
					Vertex vertex;
					vertex.pos = { attrib.vertices[3 * index.vertex_index + 0], 
						           attrib.vertices[3 * index.vertex_index + 1], 
								   attrib.vertices[3 * index.vertex_index + 2] };

					vertex.vertexNormal = glm::vec3( attrib.normals[3 * index.normal_index + 0],
													 attrib.normals[3 * index.normal_index + 1],
													 attrib.normals[3 * index.normal_index + 2]);

					vertices.push_back(vertex);
					uint32_t newIndex = static_cast<uint32_t>(vertices.size()) - 1;
					vertexIndexToUniqueIndex[vertexIndex] = newIndex;
					indices.push_back(newIndex);
				}
				else indices.push_back(vertexIndexToUniqueIndex[vertexIndex]);
			}
			uint32_t endIndices = indices.size();
			objectHash[shape.name].second = endIndices - startIndices;
		}
		/*
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
		*/

		numberOfPoints = 0;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, GROUND_PATH.c_str(), 0, true)) 
			throw std::runtime_error(warn + err);
		
		//GROUND
		for (const auto& shape : shapes) {

			for (int i = 0; i < shape.mesh.indices.size(); i += 3) {

				Vertex v1, v2, v3;

				v1.pos = { attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 2] };
				v2.pos = { attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 2] };
				v3.pos = { attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 2] };

				v1.color = { 1.0f, 1.0f, 1.0f };
				v2.color = { 1.0f, 1.0f, 1.0f };
				v3.color = { 1.0f, 1.0f, 1.0f };

				glm::vec3 triNormal = glm::normalize(glm::cross((v2.pos - v1.pos), (v3.pos - v1.pos)));

				v1.vertexNormal = v2.vertexNormal = v3.vertexNormal = triNormal;

				vertices_ground.push_back(v1);
				vertices_ground.push_back(v2);
				vertices_ground.push_back(v3);

				indices_ground.push_back(numberOfPoints++);
				indices_ground.push_back(numberOfPoints++);
				indices_ground.push_back(numberOfPoints++);
			}
		}

		numberOfPoints = 0;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, AVATAR_PATH.c_str(), 0, true))
			throw std::runtime_error(warn + err);

		//GROUND
		for (const auto& shape : shapes) {

			for (int i = 0; i < shape.mesh.indices.size(); i += 3) {

				Vertex v1, v2, v3;

				v1.pos = { attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i].vertex_index + 2] };
				v2.pos = { attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index + 2] };
				v3.pos = { attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 0], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 1], attrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index + 2] };

				v1.color = { 1.0f, 1.0f, 1.0f };
				v2.color = { 1.0f, 1.0f, 1.0f };
				v3.color = { 1.0f, 1.0f, 1.0f };

				glm::vec3 triNormal = glm::normalize(glm::cross((v2.pos - v1.pos), (v3.pos - v1.pos)));

				v1.vertexNormal = v2.vertexNormal = v3.vertexNormal = triNormal;

				vertices_avatar.push_back(v1);
				vertices_avatar.push_back(v2);
				vertices_avatar.push_back(v3);

				indices_avatar.push_back(numberOfPoints++);
				indices_avatar.push_back(numberOfPoints++);
				indices_avatar.push_back(numberOfPoints++);
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
					tempCorners.push_back(glm::vec3(tempCooords[0], tempCooords[2], tempCooords[1]));
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