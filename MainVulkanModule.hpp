#pragma once


#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>
#include <stack>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_GTC_matrix_transform
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <TinyObjectLoader/tiny_obj_loader.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "ValidationLayers.hpp"
#include "util/util_init.hpp"

