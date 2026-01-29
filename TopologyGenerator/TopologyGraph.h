#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

// 图形类型，分别为 2D 拓扑图、球体和圆环管
enum class ShapeType { TOPOLOGY_2D, SPHERE, TORUS, CUBE };

// 参数结构体
struct Parameters {
    ShapeType shape = ShapeType::TOPOLOGY_2D;

	// 选择是否美化图形
    bool beautify = false;
	// 选择是否自动旋转图形
	bool autoRotate = false;

    // 作业要求的 2D 参数
    int topology_sides = 5;
	int topology_points_per_sector = 3;

    // 3D 加分项参数

    // 子结构数量
    int sphere_sectors = 8;  
    // 每个子结构中的点数
    int sphere_points_per_sector = 5;  
    // 半球高度层数
    int sphere_layers = 3;  

	// 圆环参数，分别为主环段数、管道段数、主环半径和管道半径
    int torus_main_segments = 50;
    int torus_tube_segments = 20;
    float torus_main_radius = 1.0f;
    float torus_tube_radius = 0.3f;
};


// 拓扑图类
class TopologyGraph {
public:
	// 生成图形
    void generate(const Parameters& params);

	// 获取顶点和索引数据
    const std::vector<glm::vec3>& getVertices() const { return m_vertices; }
    const std::vector<unsigned int>& getIndices() const { return m_indices; }

	// 方便获取顶点数和边数
    size_t getVertexCount() const { return m_vertices.size(); }
    size_t getIndexCount() const { return m_indices.size(); }

private:
	// 顶点和索引数据
    std::vector<glm::vec3> m_vertices;
    std::vector<unsigned int> m_indices;

    // 图像生成函数
    void generate2DTopology(int num_sides,int points_per_sector,bool beautify);
	void generateSphere(int sectors, int points_per_sector, int layers, bool beautify);
    void generateTorus(int main_segments, int tube_segments, float main_r, float tube_r, bool beautify);
};