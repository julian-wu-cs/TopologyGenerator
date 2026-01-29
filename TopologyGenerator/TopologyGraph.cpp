#include "TopologyGraph.h"
#include <cmath>
#include <glm/gtc/constants.hpp>

void TopologyGraph::generate(const Parameters& params) {
	// 清空现有数据
    m_vertices.clear();
    m_indices.clear();

	// 根据选择的形状类型调用相应的生成函数
    switch (params.shape) {
    case ShapeType::TOPOLOGY_2D:
        generate2DTopology(params.topology_sides, params.topology_points_per_sector,params.beautify);
        break;
    case ShapeType::SPHERE:
        generateSphere(params.sphere_sectors, params.sphere_points_per_sector, params.sphere_layers, params.beautify);
        break;
    case ShapeType::TORUS:
        generateTorus(params.torus_main_segments, params.torus_tube_segments, params.torus_main_radius, params.torus_tube_radius, params.beautify);
        break;
    }
}

void TopologyGraph::generate2DTopology(int num_sides, int points_per_sector,bool beautify) {
	// 基本参数检查
    if (num_sides < 3 || points_per_sector < 1) return;

	// 圆环半径（正多边形的点都在这个圆环上）
    float outer_radius = 1.0f;

    // 添加中心点
    m_vertices.emplace_back(0.0f, 0.0f, 0.0f);

    // 添加 N 边形的 N 个外圈顶点 (顶点 1 到 N)
    for (int i = 0; i < num_sides; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / num_sides;
        float x = outer_radius * cos(angle);
        float y = outer_radius * sin(angle);
        m_vertices.emplace_back(x, y, 0.0f);
    }

    // 生成索引
    int base_vertex_offset = 1 + num_sides; // 中心点(0) + 外圈点(num_sides) 之后的第一个顶点索引
    int vertices_per_sector = points_per_sector;

    // 连接中心点到所有外圈顶点
    for (int i = 1; i <= num_sides; ++i) {
        m_indices.push_back(0); // 中心点
        m_indices.push_back(i); // 外圈顶点
    }

    // 连接外圈顶点形成多边形
    for (int i = 1; i <= num_sides; ++i) {
        m_indices.push_back(i);
        int next_vertex_index = (i == num_sides) ? 1 : i + 1;
        m_indices.push_back(next_vertex_index);
    }

	// 如果不需要美化，直接返回
	if (!beautify) return;

    // 定义一个基本单元（子结构），位于圆内部
    std::vector<glm::vec3> base_unit;
    for (int j = 0; j < points_per_sector; ++j) {
        // 将子结构点放在内部，例如在 0.5 * outer_radius 到 outer_radius 之间
        // 这里可以调整 `inner_radius` 来控制子结构的大小
        float inner_radius = outer_radius * (0.5f + 0.5f * (j + 1) / (points_per_sector + 1));
        float angle_in_sector = (j + 1) * (2.0f * glm::pi<float>() / (num_sides * 2)); // 分布在扇区的一半内
        float x = inner_radius * cos(angle_in_sector);
        float y = inner_radius * sin(angle_in_sector);
        base_unit.emplace_back(x, y, 0.0f);
    }

    // 复制并旋转子结构，放置在每个扇区内
    float angle_step = 2.0f * glm::pi<float>() / num_sides;
    for (int i = 0; i < num_sides; ++i) {
        float angle = i * angle_step;
        for (const auto& point : base_unit) {
            // 旋转点
            float x = point.x * cos(angle) - point.y * sin(angle);
            float y = point.x * sin(angle) + point.y * cos(angle);
            m_vertices.emplace_back(x, y, 0.0f);
        }
    }

    // 连接每个子结构内部的点
    for (int i = 0; i < num_sides; ++i) {
        int start = base_vertex_offset + i * vertices_per_sector;
        for (int j = 0; j < vertices_per_sector - 1; ++j) {
            m_indices.push_back(start + j);
            m_indices.push_back(start + j + 1);
        }
        // 连接子结构最后一个点到外圈顶点
        m_indices.push_back(start + vertices_per_sector - 1);
        m_indices.push_back(1 + i); // 对应外圈第 i 个点
    }

    // 连接中心点到子结构的某些点
    for (int i = 0; i < num_sides; ++i) {
        int sector_start_index = base_vertex_offset + i * vertices_per_sector;
        // 连接到子结构的第一个点
        m_indices.push_back(0); // 中心点
        m_indices.push_back(sector_start_index); // 子结构内部点
    }
}

void TopologyGraph::generateSphere(int sectors, int points_per_sector, int layers,bool beautify) {
    if (sectors < 3) return;

    // 添加球心点
    m_vertices.emplace_back(0.0f, 0.0f, 0.0f);
    int centerIndex = 0;
    float radius = 1.0f;
    float layerHeightStep = radius / layers;

    // 在XOY平面上生成正多边形的顶点作为球面点
    std::vector<int> surfaceIndices;
    float angleStep = 2.0f * glm::pi<float>() / sectors;

    for (int i = 0; i < sectors; ++i) {
        float angle = i * angleStep;
        for (int j = -layers; j <= layers; ++j) {
            float height = radius * j / layers;
            float z = height;
            float newRadius = sqrt(radius * radius - z * z);
            float x = newRadius * cos(angle);
            float y = newRadius * sin(angle);
            m_vertices.push_back(glm::vec3(x, y, z));
        }

        // 当前扇区第一个点的索引
        int idx = m_vertices.size() - (2 * layers + 1); 

        // 连接相邻的表面点形成球面环
        for (int k = 0; k < 2 * layers; ++k) {
            int current = idx + k;
            int next = idx + k + 1;
            m_indices.push_back(current);
            m_indices.push_back(next);
        }

		// 如果美化选项开启，添加扇面内的三维螺旋线
        if (beautify) {
			// 添加一半扇面内的三维螺旋线
            std::vector<int> spiralPoints1;
            for (int spiral = 0; spiral <= points_per_sector; ++spiral) {
                float spiralProgress = static_cast<float>(spiral) / points_per_sector;  // 0到1
                float spiralAngle = angle + spiralProgress * angleStep * 0.5f;  // 螺旋角度变化
                float spiralRadius = radius * spiralProgress;  // 径向距离
                float spiralZ = radius * sin(spiralProgress * glm::pi<float>()) * 0.5f;  // z方向螺旋变化

                // 计算螺旋线上的点
                float spiralX = spiralRadius * cos(spiralAngle);
                float spiralY = spiralRadius * sin(spiralAngle);

                // 添加螺旋线点
                m_vertices.push_back(glm::vec3(spiralX, spiralY, spiralZ));
                spiralPoints1.push_back(static_cast<int>(m_vertices.size() - 1));
            }

            // 连接第一条螺旋线上的相邻点
            for (int k = 0; k < spiralPoints1.size() - 1; ++k) {
                m_indices.push_back(spiralPoints1[k]);
                m_indices.push_back(spiralPoints1[k + 1]);
            }

            // 添加另一半扇面内的三维螺旋线
            std::vector<int> spiralPoints2;
            for (int spiral = 0; spiral <= points_per_sector; ++spiral) {
                float spiralProgress = static_cast<float>(spiral) / points_per_sector;  // 0到1
                float spiralAngle = angle + angleStep * 0.5f + spiralProgress * angleStep * 0.5f;  // 螺旋角度变化（从扇面中间开始）
                float spiralRadius = radius * spiralProgress;  // 径向距离
                float spiralZ = -radius * sin(spiralProgress * glm::pi<float>()) * 0.5f;  // z方向螺旋变化（相反方向）

                // 计算螺旋线上的点
                float spiralX = spiralRadius * cos(spiralAngle);
                float spiralY = spiralRadius * sin(spiralAngle);

                // 添加螺旋线点
                m_vertices.push_back(glm::vec3(spiralX, spiralY, spiralZ));
                spiralPoints2.push_back(static_cast<int>(m_vertices.size() - 1));
            }

            // 连接第二条螺旋线上的相邻点
            for (int k = 0; k < spiralPoints2.size() - 1; ++k) {
                m_indices.push_back(spiralPoints2[k]);
                m_indices.push_back(spiralPoints2[k + 1]);
            }
        }
    }
}

void TopologyGraph::generateTorus(int main_segments, int tube_segments, float main_r, float tube_r,bool beautify) {
	// 绘制基础圆环管
    for (int i = 0; i < main_segments; ++i) {
        for (int j = 0; j < tube_segments; ++j) {
            float u = (float)i / main_segments * 2.0f * glm::pi<float>();
            float v = (float)j / tube_segments * 2.0f * glm::pi<float>();
            float x = (main_r + tube_r * std::cos(v)) * std::cos(u);
            float y = (main_r + tube_r * std::cos(v)) * std::sin(u);
            float z = tube_r * std::sin(v);
            m_vertices.emplace_back(x, y, z);
            unsigned int current = i * tube_segments + j;
            unsigned int next_j = i * tube_segments + (j + 1) % tube_segments;
            unsigned int next_i = ((i + 1) % main_segments) * tube_segments + j;
            m_indices.push_back(current); m_indices.push_back(next_j);
            m_indices.push_back(current); m_indices.push_back(next_i);
        }
    }

	// 如果不需要美化，直接返回
	if (!beautify) return;

    // 在管道内部添加螺旋线点和边
    for (int i = 0; i < main_segments; i += 2) {  
        // 每隔一个主段添加内部结构
        for (int k = 0; k < 3; ++k) {  
            // 每个主段添加3条内部螺旋线
            std::vector<int> inner_spiral_points;
            float phase_offset = static_cast<float>(k) * 2.0f * glm::pi<float>() / 3.0f;  // 相位偏移

            for (int spiral = 0; spiral <= tube_segments; ++spiral) {
                float u = (float)i / main_segments * 2.0f * glm::pi<float>();
                float v = (float)spiral / tube_segments * 2.0f * glm::pi<float>() * 2.0f + phase_offset;  // 螺旋角度
                float inner_tube_r = tube_r * 0.5f; 

                float x = (main_r + inner_tube_r * std::cos(v)) * std::cos(u);
                float y = (main_r + inner_tube_r * std::cos(v)) * std::sin(u);
                float z = inner_tube_r * std::sin(v);

                m_vertices.emplace_back(x, y, z);
                inner_spiral_points.push_back(static_cast<int>(m_vertices.size() - 1));
            }

            // 连接内部螺旋线上的相邻点
            for (int p = 0; p < inner_spiral_points.size() - 1; ++p) {
                m_indices.push_back(inner_spiral_points[p]);
                m_indices.push_back(inner_spiral_points[p + 1]);
            }

            // 连接内部螺旋线到外部管壁
            for (int p = 0; p < inner_spiral_points.size(); ++p) {
                if (p < tube_segments) {
                    unsigned int outer_idx = i * tube_segments + p;
                    m_indices.push_back(inner_spiral_points[p]);
                    m_indices.push_back(outer_idx);
                }
            }
        }
    }

    // 添加管子截面内的径向线
    for (int i = 0; i < main_segments; i += 3) {  
        // 每隔几个主段添加径向结构
        for (int j = 0; j < tube_segments; j += 2) {  
            // 每隔几个管段添加径向线
            float u = (float)i / main_segments * 2.0f * glm::pi<float>();

            // 添加管子内部的径向点
            for (int radial = 1; radial <= 3; ++radial) {  
                float radial_ratio = static_cast<float>(radial) / 4.0f;  
                float inner_tube_r = tube_r * radial_ratio;
                float v = (float)j / tube_segments * 2.0f * glm::pi<float>();

                float x = (main_r + inner_tube_r * std::cos(v)) * std::cos(u);
                float y = (main_r + inner_tube_r * std::cos(v)) * std::sin(u);
                float z = inner_tube_r * std::sin(v);

                m_vertices.emplace_back(x, y, z);
                int inner_radial_idx = static_cast<int>(m_vertices.size() - 1);

                // 连接到管壁上的对应点
                unsigned int outer_idx = i * tube_segments + j;
                m_indices.push_back(inner_radial_idx);
                m_indices.push_back(outer_idx);

                // 连接内部径向点之间的层
                if (radial > 1) {
                    int prev_inner_idx = static_cast<int>(m_vertices.size() - 2); 
                    m_indices.push_back(inner_radial_idx);
                    m_indices.push_back(prev_inner_idx);
                }
            }
        }
    }

    // 添加装饰性螺旋线
    for (int spiral_num = 0; spiral_num < 4; ++spiral_num) { 
        // 4条装饰螺旋线
        std::vector<int> decorative_spiral;
        float main_spiral_offset = static_cast<float>(spiral_num) * glm::pi<float>() / 2.0f;

        for (int k = 0; k <= main_segments * 2; ++k) {  
            // 跨越2圈的螺旋
            float main_progress = static_cast<float>(k) / (main_segments * 2);

            float u = main_progress * 2.0f * glm::pi<float>() * 2.0f + main_spiral_offset;  
            float v = main_progress * 2.0f * glm::pi<float>() * 3.0f + main_spiral_offset; 
            float inner_tube_r = tube_r * 0.7f;  
            float x = (main_r + inner_tube_r * std::cos(v)) * std::cos(u);
            float y = (main_r + inner_tube_r * std::cos(v)) * std::sin(u);
            float z = inner_tube_r * std::sin(v);

            m_vertices.emplace_back(x, y, z);
            decorative_spiral.push_back(static_cast<int>(m_vertices.size() - 1));
        }

        // 连接装饰螺旋线上的相邻点
        for (int p = 0; p < decorative_spiral.size() - 1; ++p) {
            m_indices.push_back(decorative_spiral[p]);
            m_indices.push_back(decorative_spiral[p + 1]);
        }
    }
}
