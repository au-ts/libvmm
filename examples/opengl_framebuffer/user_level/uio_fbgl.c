#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "uio.h"
#include <poll.h>
#include <assert.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

void signal_ready_to_vmm() {
    printf("UIO FB|INFO: ready to receive data, faulting on VMM buffer\n");
    char command_str[64] = {0};
    sprintf(command_str, "devmem %d", UIO_INIT_ADDRESS);
    system(command_str);
}

int get_uio_map_addr(char *path, void **addr) {
    /*****************************************************************************/
    // Get address of map0 from UIO device
    size_t addr_fp = open("/sys/class/uio/uio0/maps/map0/addr", O_RDONLY);
    if (addr_fp == -1) {
        perror("Error opening file");
        return 1;
    }

    char addr_str[64]; // Buffer to hold the string representation of the address
    int addr_str_values_read = read(addr_fp, addr_str, sizeof(addr_str));

    if (addr_str_values_read <= 0) {
        perror("Error reading from file");
        close(addr_fp);
        return 2;
    }

    if (addr_str_values_read == sizeof(addr_str)) {
        fprintf(stderr, "Address string is too long to fit in address_str buffer\n");
        close(addr_fp);
        return 3;
    }

    addr_str[addr_str_values_read] = '\0'; // Null-terminate the string to be safe
    unsigned long addr_val; // Use this to hold the actual address value
    sscanf(addr_str, "%lx", &addr_val);
    *addr = (void *)addr_val;

    close(addr_fp);
}

int get_uio_map_size(char *path, size_t *size) {
    /*****************************************************************************/
    // Get size of map0 from UIO device
    size_t size_fp = open("/sys/class/uio/uio0/maps/map0/size", O_RDONLY);
    if (size_fp == -1) {
        perror("Error opening file");
        return 4;
    }

    char size_str[64]; // Buffer to hold the string representation of the size
    int size_str_values_read = read(size_fp, size_str, sizeof(size_str));

    if (size_str_values_read <= 0) {
        perror("Error reading from file");
        close(size_fp);
        return 5;
    }

    if (size_str_values_read == sizeof(size_str)) {
        fprintf(stderr, "Size string is too long to fit in size_str buffer\n");
        close(size_fp);
        return 6;
    }

    size_str[size_str_values_read] = '\0'; // Null-terminate the string to be safe
    sscanf(size_str, "%lx", size);

    close(size_fp);
}

int get_uio_map_offset(char *path, size_t *offset) {
    /*****************************************************************************/
    // Get offset of map0 from UIO device
    size_t offset_fp = open("/sys/class/uio/uio0/maps/map0/offset", O_RDONLY);
    if (offset_fp == -1) {
        perror("Error opening file");
        return 7;
    }

    char offset_str[64]; // Buffer to hold the string representation of the offset
    int offset_str_values_read = read(offset_fp, offset_str, sizeof(offset_str));

    if (offset_str_values_read <= 0) {
        perror("Error reading from file");
        close(offset_fp);
        return 8;
    }

    if (offset_str_values_read == sizeof(offset_str)) {
        fprintf(stderr, "Offset string is too long to fit in offset_str buffer\n");
        close(offset_fp);
        return 9;
    }

    offset_str[offset_str_values_read] = '\0'; // Null-terminate the string to be safe
    sscanf(offset_str, "%lx", offset);

    close(offset_fp);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoords;\n"
    "out vec2 TexCoords;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
    "   TexCoords = aTexCoords;\n"
    "}\n\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoords;\n"
    "uniform sampler2D screenTexture;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(screenTexture, TexCoords);\n"
    "}\n\0";

int main() {
    void *addr;
    size_t size;
    size_t offset;
    
    get_uio_map_addr("/sys/class/uio/uio0/maps/map0/addr", &addr);
    get_uio_map_size("/sys/class/uio/uio0/maps/map0/size", &size);
    get_uio_map_offset("/sys/class/uio/uio0/maps/map0/offset", &offset);

    printf("addr: %p\n", addr);
    printf("size: 0x%lu\n", size);
    printf("offset: 0x%lu\n", offset);
    /*****************************************************************************/
    uint32_t screensize = 0;
    /*****************************************************************************/
    // Open UIO device to allow read write and mmap
    int uio_fp = open("/dev/uio0", O_RDWR);
    if (uio_fp == -1) {
        perror("Error opening /dev/uio0");
        return 11;
    }
    printf("UIO FB|INFO: opened /dev/uio0\n");

    void *map0 = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fp, offset);

    if (map0 == MAP_FAILED) {
        perror("Error mapping uio_map0 memory");
        return 12;
    }

    printf("map0: %p\n", map0);

    /*****************************************************************************/
    // Initialise framebuffer config
    fb_config_t config = {
        .yres = SCR_HEIGHT,
        .xres = SCR_WIDTH,
        .bpp = 32,
    };
    set_fb_config(map0, config);

    void *fb_base;
    get_fb_base_addr(map0, &fb_base);
    /*****************************************************************************/
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("GLAD could not use GLFW's loader to load OpenGL functions\n");
    }

    // Other stuff
    // Set the clear color buffer value, this will ensure glClear will reset the color buffer to a dark green colour
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);

    // set up vertex and fragment shaders
    // ------------------------------------------------------------------
    // vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("vertex shader compilation failed:\n%s\n", infoLog);
    }
    // fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("fragment shader compilation failed:\n%s\n", infoLog);
    }
    // link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("shader program linking failed:\n%s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Unbind VBO, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // set up framebuffer stuff
    // ------------------------------------------------------------------
    GLuint texo;
    glGenTextures(1, &texo);
    glBindTexture(GL_TEXTURE_2D, texo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    uint32_t enable_uio_value = 1;
    // Enable UIO interrupts first, incase it is already disabled
    if (write(uio_fp, &enable_uio_value, sizeof(uint32_t)) != sizeof(uint32_t)) {
        perror("Error writing to /dev/uio0");
        close(uio_fp);
        return 13;
    }

    signal_ready_to_vmm();

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glBindTexture(GL_TEXTURE_2D, texo);

    glClear(GL_COLOR_BUFFER_BIT);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // render
        // ------
        // glClear(GL_COLOR_BUFFER_BIT); // Clear the color buffer from previous frame before drawing new stuff this frame

        // Read from device, this blocks until interrupt
        int32_t read_value;
        int32_t irq_count = 0;

        struct pollfd fds[1] = {
            { .fd = uio_fp, .events = POLLIN },
        };
        
        if (poll(fds, 1, 0) && fds->revents & POLLIN) {
            read_value = read(fds->fd, &irq_count, sizeof(irq_count));
            if (read_value != sizeof(uint32_t)) {
                perror("Error did not read 4 bytes from /dev/uio0");
                close(uio_fp);
                return 14;
            }

            printf("UIO FB|INFO: received interrupt, irq_count: %d\n", irq_count);

            // Copy from fb_base into texo
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, fb_base);

            // Enable interrupts again
            if (write(uio_fp, &enable_uio_value, sizeof(uint32_t)) != sizeof(uint32_t)) {
                perror("UIO FB|ERROR: could not write to uio_fp\n");
                close(uio_fp);
                return 15;
            }

            signal_ready_to_vmm();
        }
        
        // Draw into default FB
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();

    // close UIO device
    close(uio_fp);
    return 0;
}
