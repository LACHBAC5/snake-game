#include "shader/shader_program.hpp"
//#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <fstream>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

const int scrwidth=400, scrheight=400;

std::string readFile(const std::string& name){
    std::string out;
    int bytes=0; char buffer[64];
    int fd = open(name.c_str(), 0);
    while((bytes=read(fd, buffer, sizeof(buffer)/sizeof(char)-1))>0){
        buffer[bytes]=0;
        out += buffer;
    }
    close(fd);
    return out;
}

int genRandom(int max, int min){
    return rand()%(max-min+1)+min;
}

struct int2{
    int x=0, y=0;
};

class Snake{
    public:
    Snake(int scrwidth, int scrheight, int snake_size, lb::shader_program program) {
        _projection=glm::ortho(0.0f, (float)scrwidth, (float)scrheight, 0.0f);
        _program=program;
        _snake_size=snake_size;
        _snake.push_back({scrwidth/_snake_size/2, scrheight/_snake_size/2});
        max_slot_x=scrwidth/_snake_size; max_slot_y=scrheight/_snake_size;
        apple = _snake[0];
        spawn_apple();
    }

    void input(GLFWwindow* window){
        if(glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS){
            _direction.x=0; _direction.y=-1;
        }
        else if(glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS){
            _direction.x=0; _direction.y=1;
        }
        else if(glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS){
            _direction.x=-1; _direction.y=0;
        }
        else if(glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS){
            _direction.x=1; _direction.y=0;
        }
        else if(glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_PRESS){
            glfwSetWindowShouldClose(window, true);
        }
    }

    void update(){
        int2 head=_snake[0];
        head.x+=_direction.x; head.y+=_direction.y;

        // detect if the food has been eaten
        if(head.x==apple.x&&head.y==apple.y){
            _snake.push_back({0, 0});
            while(true){
                bool kill=true;
                for(int i = 0; i < _snake.size(); i++){
                    if(_snake[i].x==apple.x||_snake[i].y==apple.y){
                        kill=false;
                        break;
                    }
                }
                if(kill){
                    break;
                }
                spawn_apple();
            }
        }
        else if(head.x>=max_slot_x||head.x<0||head.y>=max_slot_y||head.y<0){
            _snake.erase(_snake.begin()+1, _snake.end()); head={scrwidth/_snake_size/2, scrheight/_snake_size/2};
        }

        for(int i = _snake.size()-1; i > 0; i--){
            _snake[i]=_snake[i-1];
            // detect collision
            if(_snake[i].x==head.x && _snake[i].y==head.y){
                _snake.erase(_snake.begin()+1, _snake.end()); head={scrwidth/_snake_size/2, scrheight/_snake_size/2};
                break;
            }
        }
    
        _snake[0]=head;
    }

    void draw(){
        glm::vec4 color={1.0f, 0.0f, 0.0f, 0.0f};
        // draw apple
        glm::mat4 transform = glm::translate(glm::mat4(1.0), glm::vec3((apple.x+0.5)*_snake_size, (apple.y+0.5)*_snake_size, 0.0f));
        transform = glm::scale(transform, {_snake_size/2, _snake_size/2, 0.0f});

        lb::setMat4(_program, "transform", transform);
        lb::setMat4(_program, "projection", _projection);
        lb::setVec4(_program, "color", color);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        color={1.0f, 1.0f, 1.0f, 1.0f};

        // draw snake
        for(int i = 0; i < _snake.size(); i++){
            transform = glm::translate(glm::mat4(1.0), glm::vec3((_snake[i].x+0.5)*_snake_size, (_snake[i].y+0.5)*_snake_size, 0.0f));
            transform = glm::scale(transform, {_snake_size/2, _snake_size/2, 0.0f});

            lb::setMat4(_program, "transform", transform);
            lb::setMat4(_program, "projection", _projection);
            lb::setVec4(_program, "color", color);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }

    private:
    std::vector<int2> _snake; int2 _direction = {0, 0}; int _snake_size;
    int max_slot_x, max_slot_y;
    int2 apple;
    glm::mat4 _projection; lb::shader_program _program;

    void spawn_apple(){
        apple.x=genRandom(max_slot_x-1, 0);
        apple.y=genRandom(max_slot_y-1, 0);
    }
};

int main(){
    // init glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(scrwidth, scrheight, "snake", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // init glad
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to init glad" << std::endl;
    }

    glViewport(0, 0, scrwidth, scrheight);

    // setup shaders
    lb::shader_program sh;
    lb::setup_vertex_shader(sh, readFile("snake_vertex.glsl").c_str());
    lb::setup_fragment_shader(sh, readFile("snake_fragment.glsl").c_str());
    lb::create_program(sh);

    // create square VAO
    const float body_sq[]={
         1.0f,  1.0f,
         1.0f, -1.0f,
        -1.0f, -1.0f,
        -1.0f,  1.0f
    };
    const unsigned int indices_sq[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int squareVAO, squareVBO, squareEBO;
    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &squareVBO);
    glGenBuffers(1, &squareEBO);
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(body_sq), body_sq, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, squareEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_sq), indices_sq, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);

    // create snake
    Snake snek(scrwidth, scrheight, 50, sh);
    
    double start=0, end=0;
    while(!glfwWindowShouldClose(window)){
        // update snake position every 0.25 seconds
        end=glfwGetTime();
        if(end-start>0.25){
            snek.update();
            start=end;
        }

        // process input
        glfwPollEvents();
        snek.input(window);

        // draw snake
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(sh.program);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, squareEBO);
        snek.draw();
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &squareVAO);
    glDeleteBuffers(1, &squareVBO);
    glDeleteBuffers(1, &squareEBO);

    lb::destroy_vertex_shader(sh);
    lb::destroy_fragment_shader(sh);
    lb::destroy_program(sh);

    glfwTerminate();
    return 0;
}