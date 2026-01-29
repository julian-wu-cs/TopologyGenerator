#pragma once

#include "TopologyGraph.h" 
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

class Application {
public:
    Application(unsigned int width, unsigned int height, const char* title);
    ~Application();


    void run();

private:
    // --- 成员变量 ---
    // 窗口
    GLFWwindow* m_window;
    unsigned int m_width;
    unsigned int m_height;
    std::string m_title;

    // 数据和状态
    TopologyGraph m_graph;
    Parameters m_params;

    // 渲染相关
    unsigned int m_shaderProgram;
    unsigned int m_vao, m_vbo, m_ebo;

    // 摄像机/视图控制
    glm::vec3 m_cameraPos;
    glm::vec3 m_cameraFront;
    glm::vec3 m_cameraUp;
    float m_yaw;
    float m_pitch;
    float m_fov;
    // 自动旋转的角度
    float m_autoRotationAngle = 0.0f;
    // 上一帧的时间
    float m_lastTime = 0.0f; 

    // 鼠标输入状态
    bool m_firstMouse;
    double m_lastX;
    double m_lastY;

    // 用于存储模型的旋转角度
    glm::vec2 m_modelRotation;

    //用于判断是否截图
    bool m_takeScreenshot = false;

    // --- 初始化和清理 ---
    void initWindow();
    void initGLAD();
    void initImGui();
    void initOpenGLOptions();
    void initShaders();
    void initBuffers();
    void cleanup();

    // --- 主循环中的各项任务 ---
    void mainLoop();
    void renderUI();
    void renderScene();
    void updateBuffers();
    void saveFrameToPNG();

    // --- 回调函数的成员版本 ---
    void onFramebufferSize(int width, int height);
    void onMouseMove(double xpos, double ypos);
    void onMouseScroll(double yoffset);

    // --- 静态回调，用于转发给类的成员函数 ---
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};