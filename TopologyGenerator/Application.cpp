#include "Application.h"

// 标准库
#include <iostream>
#include <vector>
#include <stdexcept>
#include <ctime>     

// 以下这个define是AI检查的时候推荐的
// 学到了这是一种防御性编程，虽然在本项目里没有用到std::min/max，但养成习惯总是好的
// 这个习惯可以防止 windows.h 定义 min 和 max 宏，避免与 std::min 和 std::max 冲突
#define NOMINMAX 
#include <windows.h> 

// 第三方库
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


// 构造函数
Application::Application(unsigned int width, unsigned int height, const char* title)
    : m_window(nullptr), m_width(width), m_height(height), m_title(title),
    m_shaderProgram(0), m_vao(0), m_vbo(0), m_ebo(0),
    m_cameraPos(0.0f, 0.0f, 4.0f), m_cameraFront(0.0f, 0.0f, -1.0f), m_cameraUp(0.0f, 1.0f, 0.0f),
    m_yaw(-90.0f), m_pitch(0.0f), m_fov(45.0f),
    m_firstMouse(true), m_lastX(width / 2.0), m_lastY(height / 2.0),
    m_modelRotation(0.0f, 0.0f)
{
}

// 这也是AI推荐的，主要用于释放资源，防止内存泄漏
Application::~Application() {
    cleanup();
}

void Application::run() {
    try {
        initWindow();
        initGLAD();
        initOpenGLOptions();
        initImGui();
        initShaders();
        initBuffers();

        m_graph.generate(m_params);
        updateBuffers();

        mainLoop();
    }
    catch (const std::exception& e) {
        std::cerr << "Error during application lifecycle: " << e.what() << std::endl;
		//按任意键退出
		std::cout << "Press any key to exit..." << std::endl;
		std::cin.get();

        // 这里不用写cleanup了，因为在析构函数中会自动调用 cleanup
    }
}

// 初始化函数
void Application::initWindow() {
    if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(m_window);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(m_window, mouse_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
}

void Application::initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Application::initOpenGLOptions() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
}

void Application::initShaders() {
    const char* vShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 mvp;
        void main() {
            gl_Position = mvp * vec4(aPos, 1.0);
        }
    )";
    const char* fShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 objectColor;
        void main() {
            FragColor = vec4(objectColor, 1.0f);
        }
    )";

    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vShaderSrc, NULL);
    glCompileShader(vShader);

    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fShaderSrc, NULL);
    glCompileShader(fShader);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vShader);
    glAttachShader(m_shaderProgram, fShader);
    glLinkProgram(m_shaderProgram);

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

void Application::initBuffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBindVertexArray(0);
}

// --- 主循环 ---
void Application::mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
		// 处理所有待处理的窗口事件
        glfwPollEvents();

		// 清除颜色缓冲区和深度缓冲区
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 渲染用户界面
        renderUI();

        // 渲染 3D 场景
        renderScene();

        // 将 ImGui 构建的 UI 数据提交给 OpenGL 进行渲染
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// 如果需要截图，执行截图操作
        if (m_takeScreenshot) {
            saveFrameToPNG();
            // 重置标志位，防止每帧都截图
            m_takeScreenshot = false; 
        }

		// 交换前后缓冲区，显示渲染结果
        glfwSwapBuffers(m_window);
    }
}

// 渲染用户界面（为什么用英文，因为Imgui对中文的支持很不好）
void Application::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool needs_update = false;

    // 设置窗口大小
    ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
    // 固定位置在左上角
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("control panel");
    ImGui::Text("Right click and drag to rotate objects,\nscroll wheel to zoom in and out.");
    ImGui::Separator();

    const char* items[] = { "2D Topology Map (basic)", "Sphere", "torus"};

	// 这里用于实现选择不同的结构类型
	ImGui::Text("Select structure type:");
    if (ImGui::Combo("##00", (int*)&m_params.shape, items, IM_ARRAYSIZE(items))) needs_update = true;

	// 以下用于实现是否勾选图像美化选项（即是否添加内部螺旋线结构等）
    // 保存旧状态
    bool old_beautify_state = m_params.beautify;

	// 创建复选框且改变文字颜色以突出显示
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.8f, 1.0f));
    ImGui::Checkbox("Beautify", &m_params.beautify);
	ImGui::Checkbox("Automatic rotation", &m_params.autoRotate);
    ImGui::PopStyleColor();

    // 检测状态是否发生了变化
    if (old_beautify_state != m_params.beautify) {
        needs_update = true;  // 状态改变时触发更新
    }

	// 根据选择的结构类型显示不同的参数选项
    switch (m_params.shape) {
    case ShapeType::TOPOLOGY_2D:
        ImGui::Text("Number of substructures:");
		//如下的##01等是为了给每个滑动条一个独特的ID，防止冲突
		//因为ImGui的每个控件都需要一个唯一的标识符
        if (ImGui::SliderInt("##01", &m_params.topology_sides, 3, 100)) needs_update = true;
		// 如果勾选了美化选项，显示更多参数
        if (m_params.beautify) {
            ImGui::Text("Number of points in each substructure:");
            if (ImGui::SliderInt("##02", &m_params.topology_points_per_sector, 1, 50)) needs_update = true;
        }
        break;
    case ShapeType::SPHERE:
        ImGui::Text("Number of substructures:");
		if (ImGui::SliderInt("##03", &m_params.sphere_sectors, 3, 100)) needs_update = true;
		// 如果勾选了美化选项，显示更多参数
        if (m_params.beautify) {
            ImGui::Text("Number of points in each substructure:");
            if (ImGui::SliderInt("##04", &m_params.sphere_points_per_sector, 1, 50)) needs_update = true;
        }
        ImGui::Text("Height layer:");
        if (ImGui::SliderInt("##05", &m_params.sphere_layers, 2, 50)) needs_update = true;
        break;
    case ShapeType::TORUS:
        ImGui::Text("Number of main ring segments:");
        if (ImGui::SliderInt("##06", &m_params.torus_main_segments, 4, 100)) needs_update = true;
        ImGui::Text("Number of pipe ring segments:");
        if (ImGui::SliderInt("##07", &m_params.torus_tube_segments, 3, 50)) needs_update = true;
        ImGui::Text("Main ring radius:");
        if (ImGui::SliderFloat("##08", &m_params.torus_main_radius, 0.5f, 2.0f)) needs_update = true;
        ImGui::Text("Pipe ring radius:");
        if (ImGui::SliderFloat("##09", &m_params.torus_tube_radius, 0.1f, 1.0f)) needs_update = true;
        break;
    }

    ImGui::Separator();
    ImGui::Text("Current number of points: %zu", m_graph.getVertexCount());
	// 因为边是成对存储的，所以边的数量是索引数量除以2
    ImGui::Text("Current number of edges: %zu", m_graph.getIndexCount() / 2);
    
	// 添加截图按钮
    ImGui::Spacing();
    if (ImGui::Button("save as PNG image")) {
		m_takeScreenshot = true;// 设置截图标志
    }

    ImGui::End();

	// 渲染 ImGui 界面
    ImGui::Render();

	// 如果参数有变化，重新生成图形并更新缓冲区
    if (needs_update) {
        m_graph.generate(m_params);
        updateBuffers();
    }
}

void Application::renderScene() {
    glUseProgram(m_shaderProgram);

    // 视图和投影矩阵
    glm::mat4 view = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
    glm::mat4 proj = glm::perspective(glm::radians(m_fov), (float)m_width / m_height, 0.1f, 100.0f);

    // 模型矩阵现在根据鼠标输入动态计算
    glm::mat4 model = glm::mat4(1.0f);

    float currentTime = glfwGetTime();
    float deltaTime = currentTime - m_lastTime;
    m_lastTime = currentTime;

    // 自动旋转
    if (m_params.autoRotate) {
        // 每秒 8 度（慢速）
        m_autoRotationAngle += deltaTime * 8.0f; 
        if (m_autoRotationAngle >= 360.0f) {
            m_autoRotationAngle -= 360.0f;
        }
        model = glm::rotate(model, glm::radians(m_autoRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // 手动旋转
    model = glm::rotate(model, glm::radians(m_modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(m_modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

    // 计算最终的 MVP 矩阵
    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(m_vao);

    // 绘制边
    glUniform3f(glGetUniformLocation(m_shaderProgram, "objectColor"), 0.7f, 0.7f, 0.7f);
    glDrawElements(GL_LINES, static_cast<GLsizei>(m_graph.getIndexCount()), GL_UNSIGNED_INT, 0);

    // 绘制节点
    glUniform3f(glGetUniformLocation(m_shaderProgram, "objectColor"), 0.2f, 0.8f, 0.8f);
    glPointSize(5.0f);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_graph.getVertexCount()));

    glBindVertexArray(0);
}

void Application::updateBuffers() {
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_graph.getVertexCount() * sizeof(glm::vec3), m_graph.getVertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_graph.getIndexCount() * sizeof(unsigned int), m_graph.getIndices().data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void Application::saveFrameToPNG() {
    // 截图要保存的文件夹名称
    const std::string directory = "screenshots";

	// 检查文件夹是否存在，如果不存在则创建
    DWORD attributes = GetFileAttributesA(directory.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        // 文件夹不存在，尝试创建
        if (CreateDirectoryA(directory.c_str(), NULL)) {
            std::cout << "创建截图文件夹: " << directory << std::endl;
        }
        else {
            std::cerr << "创建截图文件夹失败!" << std::endl;
            return; // 创建失败则不继续
        }
    }

	// 生成基于当前时间的唯一文件名，主要是解决用户多次截图的覆盖问题
    // 获取当前时间
    time_t now = time(0);

    // 将时间转换为本地时间结构体
    tm t_struct;
    localtime_s(&t_struct, &now); 

    // 定义一个缓冲区来存储格式化后的时间字符串
    char timestamp_buffer[80];

    // 使用 strftime 格式化时间
    strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y-%m-%d_%H-%M-%S", &t_struct);

    // 组合成完整的文件路径
    std::string filename = directory + "/topology_" + timestamp_buffer + ".png";

    // 读取像素并保存文件 
    std::vector<unsigned char> pixels(m_width * m_height * 3);
    glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    stbi_flip_vertically_on_write(1);
    if (stbi_write_png(filename.c_str(), m_width, m_height, 3, pixels.data(), m_width * 3)) {
        std::cout << "截图已保存到: " << filename << std::endl;
    }
    else {
        std::cerr << "错误: 无法保存截图到 " << filename << std::endl;
    }
}

// 清理函数
void Application::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    glDeleteProgram(m_shaderProgram);

    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

// 静态回调
void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onFramebufferSize(width, height);
}

void Application::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onMouseMove(xpos, ypos);
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onMouseScroll(yoffset);
}

// 成员回调
void Application::onFramebufferSize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

void Application::onMouseMove(double xpos, double ypos) {
    // 仅在鼠标右键按下时才处理旋转
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) {
        m_firstMouse = true;
        return;
    }

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    // 计算鼠标自上一帧以来的位移
    double xoffset = xpos - m_lastX;
    double yoffset = m_lastY - ypos;
    m_lastX = xpos;
    m_lastY = ypos;

    // 设置旋转灵敏度
    float sensitivity = 0.5f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // 【核心修改】更新模型的旋转角度，而不是相机
    m_modelRotation.y += static_cast<float>(xoffset);
    m_modelRotation.x += static_cast<float>(yoffset);
}

void Application::onMouseScroll(double yoffset) {
    m_fov -= (float)yoffset;
    if (m_fov < 1.0f) m_fov = 1.0f;
    if (m_fov > 60.0f) m_fov = 60.0f;
}
