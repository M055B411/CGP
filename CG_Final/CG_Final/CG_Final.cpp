// 표준 라이브러리
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

// OpenGL 관련 헤더
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>

// GLM 관련 헤더
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/constants.hpp>
#include <gl/glm/gtx/transform.hpp>

// 이미지 로딩 관련
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLvoid DrawScene(GLvoid);
GLvoid Reshape(int, int);

// 전역 변수 추가
GLuint startPageTexture;


// 메뉴 버튼 구조체 추가
struct Button {
    float x, y, width, height;
    const char* text;
    bool isHovered;
};

// 전역 변수 추가
Button startButton = { 1280 / 2 - 100, 720 / 2 - 50, 200, 50, "Game Start", false };
Button exitButton = { 1280 / 2 - 100, 720 / 2 - 120, 200, 50, "Exit", false };

// 게임 상태 열거형 추가
enum GameState {
    MAIN_MENU,
    POT_SELECT,
    GAME_PLAY
};

// 전역 변수 수정
GameState gameState = MAIN_MENU;
int currentPotIndex = 0;
const int NUM_POTS = 2;  // 선택 가능한 화분 개수

int currentPlantindex = 0;
const int NUM_PLANTS = 7;

// 화분 선택 화면의 버튼 추가
Button leftArrowButton = { 100, 720 / 2 - 25, 50, 50, "<", false };
Button rightArrowButton = { 1280 - 150, 720 / 2 - 25, 50, 50, ">", false };
Button confirmButton = { 1280 / 2 - 100, 100, 200, 50, "Select Pot", false };

// 처리 로드 함수 추가
void loadTexture(const char* filename, GLuint* texture) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!image) {
        std::cerr << "이미지 로드 실패: " << filename << std::endl;
        return;
    }

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    stbi_image_free(image);
}

// 마우스 콜백 함수 추가
bool click = false;
int prex, prey = 0;
float TotalRot = 1.0f;
void mouse(int button, int state, int x, int y);

// 마우스 움직임 감지 함수 추가

static int mousex, mousey;
void mouseMove(int x, int y) {
    int convertedY = 720 - y;

    switch (gameState) {
    case MAIN_MENU:
        startButton.isHovered = (x >= startButton.x && x <= startButton.x + startButton.width &&
            convertedY >= startButton.y && convertedY <= startButton.y + startButton.height);
        exitButton.isHovered = (x >= exitButton.x && x <= exitButton.x + exitButton.width &&
            convertedY >= exitButton.y && convertedY <= exitButton.y + exitButton.height);
        break;

    case POT_SELECT:
        leftArrowButton.isHovered = (x >= leftArrowButton.x && x <= leftArrowButton.x + leftArrowButton.width &&
            convertedY >= leftArrowButton.y && convertedY <= leftArrowButton.y + leftArrowButton.height);
        rightArrowButton.isHovered = (x >= rightArrowButton.x && x <= rightArrowButton.x + rightArrowButton.width &&
            convertedY >= rightArrowButton.y && convertedY <= rightArrowButton.y + rightArrowButton.height);
        confirmButton.isHovered = (x >= confirmButton.x && x <= confirmButton.x + confirmButton.width &&
            convertedY >= confirmButton.y && convertedY <= confirmButton.y + confirmButton.height);
        break;

    case GAME_PLAY:
        mousex = x;
        mousey = convertedY;
        break;
    }
    glutPostRedisplay();
}

// 전역 변수 ���가 (파일 상단에 추가)
float cameraAngle = 0.0f;  // 카메라 회전 각도

// mousedrag 함수 수정
void mousedrag(int x, int y) {
    int convertedY = 720 - y;

    switch (gameState) {
    case MAIN_MENU:
        break;
    case POT_SELECT:
        break;
    case GAME_PLAY:
        if (click) {
            // 초당 90도 전하도록 설정
            float rotationSpeed = 90.0f;  // 초당 회전 각도
            float deltaTime = 1.0f / 60.0f;  // 프레임당 시간 (60fps 기준)
            cameraAngle += rotationSpeed * deltaTime * ((prex > x) ? 1 : -1);
            prex = x;
        }
        break;
    }
    glutPostRedisplay();
}

// 전역 변수 추가
float cameraDistance = 3.0f;  // 카메라 거리 (기본값 3.0)
const float MIN_DISTANCE = 2.0f;  // 최소 거리
const float MAX_DISTANCE = 10.0f;  // 최대 거리

// mousewheel 함수 수정
void mousewheel(int button, int dir, int x, int y) {
    if (dir > 0) {
        // 휠 위로 (줌 아웃)
        if (cameraDistance < MAX_DISTANCE)
            cameraDistance += 0.5f;
    }
    else if (dir < 0) {
        // 휠 아래로 (줌 인)
        if (cameraDistance > MIN_DISTANCE)
            cameraDistance -= 0.5f;
    }
    glutPostRedisplay();
}



// 키보드 콜백 함수 추가
static double keyx, keyz;
static bool plantmode = false;
void keyboard(unsigned char key, int x, int y);


// 특수 키 콜백 함수 추가
void specialKeys(int key, int x, int y) {
    if (gameState == POT_SELECT) {
        switch (key) {
        case GLUT_KEY_LEFT:
            currentPotIndex = (currentPotIndex - 1 + NUM_POTS) % NUM_POTS;
            break;
        case GLUT_KEY_RIGHT:
            currentPotIndex = (currentPotIndex + 1) % NUM_POTS;
            break;
        }
        glutPostRedisplay();
    }
}

// Material 구조체 추가
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    std::string texturePath;
    bool hasTexture;
};

// Model 구조체에 Material 추가
struct Model {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> faces;
    Material material;
    bool isLoaded;
    GLuint textureId;
    float height;
};

// Plant 구조체 수정
struct Plant {
    Model Model;
    float x, y, z;  // 위치
    float rx, ry, rz;  // 회전
    float spawnTime;  // 생성 시간
    bool isActive;  // 현재 배치된 상태인지
    float scale;  // 크기 비율
};

// 전역 변수 추가
std::vector<Model> potModels;  // 화분 모델들을 저장할 벡터

std::vector<Model> plantModels;

// 전역 변수 추가
std::vector<Plant> placedPlants;  // 배치된 식물들을 저장할 벡터

// MTL 파일 로드 함수
void loadMTL(const char* filename, Material& material) {
    FILE* file;
    char line[1000];
    float x, y, z;

    fopen_s(&file, filename, "r");
    if (!file) {
        std::cerr << "MTL 파일 열기 오류: " << filename << std::endl;
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        // 주석 무시
        if (line[0] == '#') continue;

        // ambient color
        if (strncmp(line, "Ka", 2) == 0) {
            sscanf_s(line + 2, "%f %f %f", &x, &y, &z);
            material.ambient = glm::vec3(x, y, z);
        }
        // diffuse color
        else if (strncmp(line, "Kd", 2) == 0) {
            sscanf_s(line + 2, "%f %f %f", &x, &y, &z);
            material.diffuse = glm::vec3(x, y, z);
        }
        // specular color
        else if (strncmp(line, "Ks", 2) == 0) {
            sscanf_s(line + 2, "%f %f %f", &x, &y, &z);
            material.specular = glm::vec3(x, y, z);
        }
        // shininess
        else if (strncmp(line, "Ns", 2) == 0) {
            sscanf_s(line + 2, "%f", &x);
            material.shininess = x;
        }
        // diffuse texture map
        else if (strncmp(line, "map_Kd", 6) == 0) {
            char texturePath[256];
            sscanf_s(line + 7, "%s", texturePath, (unsigned)sizeof(texturePath));
            material.texturePath = texturePath;
            material.hasTexture = true;
        }
    }

    fclose(file);
}

// OBJ 파일 로드 함수 수정
void loadFromObjFile(const char* filename, Model& model) {
    FILE* file;
    char line[1000];
    float x, y, z;
    unsigned int v[3], vt[3], vn[3];

    fopen_s(&file, filename, "r");
    if (!file) {
        std::cerr << "파일 열기 오류: " << filename << std::endl;
        return;
    }

    // 기존 데이터 초기화
    model.vertices.clear();
    model.faces.clear();
    model.normal.clear();
    model.texCoords.clear();

    // 파일 파싱
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == 'o' || strlen(line) == 0 || line[0] == 's')
            continue;

        if (line[0] == 'v' && line[1] == ' ') {
            if (sscanf_s(line + 2, "%f %f %f", &x, &y, &z) == 3) {
                model.vertices.push_back(glm::vec3(x, y, -z));  // z축 반전
            }
        }
        else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
            if (sscanf_s(line + 3, "%f %f %f", &x, &y, &z) == 3) {
                model.normal.push_back(glm::vec3(x, y, -z));  // z축 반전
            }
        }
        else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
            if (sscanf_s(line + 3, "%f %f", &x, &y) == 2) {
                model.texCoords.push_back(glm::vec2(x, 1.0f - y));  // v표 반전
            }
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            unsigned int v[4], vt[4], vn[4];
            int numVertices = 0;

            // 먼저 4개의 정점을 읽어보기 시도
            if (sscanf_s(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
                &v[0], &vt[0], &vn[0],
                &v[1], &vt[1], &vn[1],
                &v[2], &vt[2], &vn[2],
                &v[3], &vt[3], &vn[3]) == 12) {
                // 사각형 면 (4개의 정점)
                numVertices = 4;
            }
            else if (sscanf_s(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u",
                &v[0], &vt[0], &vn[0],
                &v[1], &vt[1], &vn[1],
                &v[2], &vt[2], &vn[2]) == 9) {
                // 삼각형 면 (3개의 정점)
                numVertices = 3;
            }

            // 첫 번째 삼각형 (0-1-2) 추가
            for (int i = 0; i < 3; i++) {
                model.faces.push_back(v[i] - 1);
                if (!model.texCoords.empty()) {
                    model.texCoords.push_back(model.texCoords[vt[i] - 1]);
                }
                if (!model.normal.empty()) {
                    model.normal.push_back(model.normal[vn[i] - 1]);
                }
            }

            // 사각형인 경우 두 번째 삼각형 (2-3-0) 추가
            if (numVertices == 4) {
                // 2-3-0 삼각형 추가
                model.faces.push_back(v[2] - 1);
                model.faces.push_back(v[3] - 1);
                model.faces.push_back(v[0] - 1);
                
                if (!model.texCoords.empty()) {
                    model.texCoords.push_back(model.texCoords[vt[2] - 1]);
                    model.texCoords.push_back(model.texCoords[vt[3] - 1]);
                    model.texCoords.push_back(model.texCoords[vt[0] - 1]);
                }
                if (!model.normal.empty()) {
                    model.normal.push_back(model.normal[vn[2] - 1]);
                    model.normal.push_back(model.normal[vn[3] - 1]);
                    model.normal.push_back(model.normal[vn[0] - 1]);
                }
            }
        }
        else if (strncmp(line, "mtllib", 6) == 0) {
            char mtlPath[256];
            char fullPath[512];
            sscanf_s(line + 7, "%s", mtlPath, (unsigned)sizeof(mtlPath));
            sprintf_s(fullPath, "Models/%s", mtlPath);  // Models 폴더 경로 추가
            loadMTL(fullPath, model.material);
        }
    }
    fclose(file);

    // 파일 파싱이 끝난 후 모델의 높이 계산
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (const auto& vertex : model.vertices) {
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }
    
    model.height = maxY - minY;  // Model 구조체에 height 멤버 추가 필요
    
    model.isLoaded = true;
}

// 전역 변수 추가 (파일 상단)
std::chrono::steady_clock::time_point start_time;
float elapsed_time = 0.0f;
const int TIMER_INTERVAL = 1000;  // 타이머 간격 (밀리초)

// 전역 변수 추가
float lastFrameTime = 0.0f;
float deltaTime = 0.0f;

// Timer 함수 수정
void Timer(int value) {
    auto current_time = std::chrono::steady_clock::now();
    std::chrono::duration<float> duration = current_time - start_time;
    elapsed_time = duration.count();
    
    deltaTime = elapsed_time - lastFrameTime;
    lastFrameTime = elapsed_time;
    
    glutTimerFunc(TIMER_INTERVAL, Timer, 1);
    glutPostRedisplay();
}

// 식물 배치 함수 추가
void placePlant(int plantIndex, float x, float y, float z) {
    Plant newPlant;
    newPlant.Model = plantModels[plantIndex];
    newPlant.x = x;
    newPlant.y = y;
    newPlant.z = z;
    newPlant.rx = 0;
    newPlant.ry = 0;
    newPlant.rz = 0;
    newPlant.spawnTime = elapsed_time;
    newPlant.isActive = true;
    newPlant.scale = 0.6f;
    
    placedPlants.push_back(newPlant);
}

// 화분의 높이를 구하는 함수 수정
float getPotHeight(const Model& potModel) {
    // 모델이 로드되지 않았거나 vertices가 비어있는 경우
    if (!potModel.isLoaded || potModel.vertices.empty()) {
        std::cout << "Warning: Pot model is not loaded or empty" << std::endl;
        return 0.5f;  // 기본 높이값 반환
    }

    float minY = potModel.vertices[0].y;
    float maxY = potModel.vertices[0].y;
    
    for (const auto& vertex : potModel.vertices) {
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }
    
    float height = maxY - minY;
    return height > 0 ? height : 0.5f;  // 이가 0 이하면 기본 반환
}

// 전역 변수 추가
int score = 0;  // 점수 저장 변수

// getPotRadius 함수 추가
float getPotRadius(const Model& potModel) {
    if (!potModel.isLoaded || potModel.vertices.empty()) {
        return 0.5f;  // 기본값
    }

    float maxRadius = 0.0f;
    for (const auto& vertex : potModel.vertices) {
        float radius = sqrt(vertex.x * vertex.x + vertex.z * vertex.z);
        maxRadius = std::max(maxRadius, radius);
    }
    
    return maxRadius;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("OpenGL");

    if (glewInit() != GLEW_OK) {
        std::cerr << "Unable to initialize GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "GLEW Initialized" << std::endl;
    }

    // 텍스처 모델 로드
    potModels.resize(NUM_POTS);
    loadFromObjFile("Models/Pot_01.obj", potModels[0]);
    loadFromObjFile("Models/Pot_02.obj", potModels[1]);

    // 텍스처 로드
    loadTexture("Textures/MainTitle.png", &startPageTexture);
    loadTexture("Textures/Pot_01.png", &potModels[0].textureId);
    loadTexture("Textures/Pot_02.png", &potModels[1].textureId);

    plantModels.resize(NUM_PLANTS);
    loadFromObjFile("Models/Cactus_01.obj", plantModels[0]);
    loadFromObjFile("Models/Flower_01.obj", plantModels[1]);
    loadFromObjFile("Models/Flower_02.obj", plantModels[2]);
    loadFromObjFile("Models/Grass_01.obj", plantModels[3]);
    loadFromObjFile("Models/Grass_02.obj", plantModels[4]);
    loadFromObjFile("Models/Tree_01.obj", plantModels[5]);
    loadFromObjFile("Models/Tree_01.obj", plantModels[6]);

    loadTexture("Textures/Cactus_01.png", &plantModels[0].textureId);
    loadTexture("Textures/Flower_01.png", &plantModels[1].textureId);
    loadTexture("Textures/Flower_03.png", &plantModels[2].textureId);
    loadTexture("Textures/Grass_01.png", &plantModels[3].textureId);
    loadTexture("Textures/Grass_01.png", &plantModels[4].textureId);
    loadTexture("Textures/Tree_01.png", &plantModels[5].textureId);
    loadTexture("Textures/Tree_01.png", &plantModels[6].textureId);

    // 시작 시간 설정
    start_time = std::chrono::steady_clock::now();
    
    glutDisplayFunc(DrawScene);
    glutReshapeFunc(Reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mousedrag);
    glutPassiveMotionFunc(mouseMove);
    glutMouseWheelFunc(mousewheel);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    
    // 타이머 콜백 등록
    glutTimerFunc(TIMER_INTERVAL, Timer, 1);
    
    glutMainLoop();
    return 0;
}

GLvoid DrawScene(GLvoid) {
    // 배경색을 밝은 회색으로 변경
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 변수 언을 switch문 앞으로 이동
    GLfloat light_position[] = { 5.0f, 5.0f, 5.0f, 1.0f };
    GLfloat light_ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    GLfloat pot_colors[2][4] = {
        {0.8f, 0.4f, 0.4f, 1.0f},  // 빨간색 계열
        {0.4f, 0.8f, 0.4f, 1.0f},  // 초록색 계열
    };

    GLfloat mat_ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_shininess[] = { 100.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    GLUquadricObj* quadric = nullptr;

    switch (gameState) {
    case MAIN_MENU:
        // 메인 메뉴 렌더링
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 1280, 0, 720);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // 배경 이미지
        glEnable(GL_TEXTURE_2D);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBindTexture(GL_TEXTURE_2D, startPageTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1280, 0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1280, 720);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0, 720);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        // 버튼 그리기
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 시작 버튼
        if (startButton.isHovered)
            glColor4f(0.8f, 0.8f, 0.8f, 0.7f);
        else
            glColor4f(0.5f, 0.5f, 0.5f, 0.7f);

        glBegin(GL_QUADS);
        glVertex2f(startButton.x, startButton.y);
        glVertex2f(startButton.x + startButton.width, startButton.y);
        glVertex2f(startButton.x + startButton.width, startButton.y + startButton.height);
        glVertex2f(startButton.x, startButton.y + startButton.height);
        glEnd();

        // 종료 버튼
        if (exitButton.isHovered)
            glColor4f(0.8f, 0.8f, 0.8f, 0.7f);
        else
            glColor4f(0.5f, 0.5f, 0.5f, 0.7f);

        glBegin(GL_QUADS);
        glVertex2f(exitButton.x, exitButton.y);
        glVertex2f(exitButton.x + exitButton.width, exitButton.y);
        glVertex2f(exitButton.x + exitButton.width, exitButton.y + exitButton.height);
        glVertex2f(exitButton.x, exitButton.y + exitButton.height);
        glEnd();

        glDisable(GL_BLEND);

        // 버튼 텍스트
        glColor3f(1.0f, 1.0f, 1.0f);

        glRasterPos2i(startButton.x + 60, startButton.y + 20);
        for (const char* c = startButton.text; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        glRasterPos2i(exitButton.x + 80, exitButton.y + 20);
        for (const char* c = exitButton.text; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
        break;

    case POT_SELECT:
        // 렌더링 모드 설정 추가
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 와이어프레임이 아 채워진 폴리곤으로 렌더링

        // 법선 벡터 자동 정규화 활성화
        glEnable(GL_NORMALIZE);

        // 조명 설정 수
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        // 조명 설정 적용 (변수 선언 제거)
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

        // 3D 투영 설정
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)1280 / 720, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0, 1, 3,  // 카메라 위치를 더 가깝게
            0, 0, 0,   // 보 지점
            0, 1, 0);  // 업 벡터

        // 화분 모델 렌더링
        if (potModels[currentPotIndex].isLoaded) {
            // 재질 설정
            Material& mat = potModels[currentPotIndex].material;
            GLfloat ambient[] = { mat.ambient.x, mat.ambient.y, mat.ambient.z, 1.0f };
            GLfloat diffuse[] = { mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, 1.0f };
            GLfloat specular[] = { mat.specular.x, mat.specular.y, mat.specular.z, 1.0f };
            GLfloat shininess[] = { mat.shininess };

            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

            // 텍스처 설정
            if (mat.hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, potModels[currentPotIndex].textureId);
            }

            glPushMatrix();
            glRotatef(30, 0, 1, 0);
            glScalef(0.5f, 0.5f, 0.5f);  // 0.1에서 0.5로 수정

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < potModels[currentPotIndex].faces.size(); i++) {
                unsigned int vertexIndex = potModels[currentPotIndex].faces[i];

                // 법선 벡터
                if (vertexIndex < potModels[currentPotIndex].normal.size()) {
                    glm::vec3 normal = potModels[currentPotIndex].normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
                }

                // 텍스처 좌표
                if (vertexIndex < potModels[currentPotIndex].texCoords.size()) {
                    glm::vec2 texCoord = potModels[currentPotIndex].texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                // 정점
                glm::vec3 vertex = potModels[currentPotIndex].vertices[vertexIndex];
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            glEnd();
            glPopMatrix();

            if (mat.hasTexture) {
                glDisable(GL_TEXTURE_2D);
            }
            glDisable(GL_NORMALIZE);  // 정규화 비활성화
        }

        // 2D 오버레이 설정
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 1280, 0, 720);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // 살표 그리기
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 왼쪽 화살표
        if (leftArrowButton.isHovered)
            glColor4f(0.8f, 0.8f, 0.8f, 0.7f);
        else
            glColor4f(0.5f, 0.5f, 0.5f, 0.7f);

        glBegin(GL_QUADS);
        glVertex2f(leftArrowButton.x, leftArrowButton.y);
        glVertex2f(leftArrowButton.x + leftArrowButton.width, leftArrowButton.y);
        glVertex2f(leftArrowButton.x + leftArrowButton.width, leftArrowButton.y + leftArrowButton.height);
        glVertex2f(leftArrowButton.x, leftArrowButton.y + leftArrowButton.height);
        glEnd();

        // 오른 화살표
        if (rightArrowButton.isHovered)
            glColor4f(0.8f, 0.8f, 0.8f, 0.7f);
        else
            glColor4f(0.5f, 0.5f, 0.5f, 0.7f);

        glBegin(GL_QUADS);
        glVertex2f(rightArrowButton.x, rightArrowButton.y);
        glVertex2f(rightArrowButton.x + rightArrowButton.width, rightArrowButton.y);
        glVertex2f(rightArrowButton.x + rightArrowButton.width, rightArrowButton.y + rightArrowButton.height);
        glVertex2f(rightArrowButton.x, rightArrowButton.y + rightArrowButton.height);
        glEnd();

        // 확인 버튼
        if (confirmButton.isHovered)
            glColor4f(0.8f, 0.8f, 0.8f, 0.7f);
        else
            glColor4f(0.5f, 0.5f, 0.5f, 0.7f);

        glBegin(GL_QUADS);
        glVertex2f(confirmButton.x, confirmButton.y);
        glVertex2f(confirmButton.x + confirmButton.width, confirmButton.y);
        glVertex2f(confirmButton.x + confirmButton.width, confirmButton.y + confirmButton.height);
        glVertex2f(confirmButton.x, confirmButton.y + confirmButton.height);
        glEnd();

        glDisable(GL_BLEND);

        // 버튼 텍스트
        glColor3f(1.0f, 1.0f, 1.0f);

        glRasterPos2i(leftArrowButton.x + 20, leftArrowButton.y + 20);
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '<');

        glRasterPos2i(rightArrowButton.x + 20, rightArrowButton.y + 20);
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '>');

        glRasterPos2i(confirmButton.x + 60, confirmButton.y + 20);
        for (const char* c = confirmButton.text; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
        break;

    case GAME_PLAY:
        // 게임 플레이 렌더링 코드는 나머지 구현
        // 렌더링 모드 설정 추가
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 와이어프레임이 아닌 채워진 폴리곤으로 렌더링

        // 법선 벡터 자동 정규화 활성화
        glEnable(GL_NORMALIZE);

        // 조명 설정 수
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        // 조명 설정 적용 (변수 선언 제거)
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

        // 3D 투영 설정
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)1280 / 720, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // 카메라 위치 계산
        float camX = cameraDistance * sin(glm::radians(cameraAngle));
        float camZ = cameraDistance * cos(glm::radians(cameraAngle));
        float camY = cameraDistance * 1.2f;  // 0.8f에서 1.2f로 증가

        // 보는 지점의 Y값도 거리에 따라 조정
        float lookAtY = -cameraDistance * 0.1f;  // 거리가 멀어질수록 더 아래를 봄

        gluLookAt(camX, camY, camZ,  // 카메라 위치
                  0, lookAtY, 0,   // 보는 지점
                  0, 1, 0);  // 업 벡터

        if (potModels[currentPotIndex].isLoaded) {
            // 재질 설정
          
            Material& mat = potModels[currentPotIndex].material;
            GLfloat ambient[] = { mat.ambient.x, mat.ambient.y, mat.ambient.z, 1.0f };
            GLfloat diffuse[] = { mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, 1.0f };
            GLfloat specular[] = { mat.specular.x, mat.specular.y, mat.specular.z, 1.0f };
            GLfloat shininess[] = { mat.shininess };

            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

            // 텍스처 설정
            if (mat.hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, potModels[currentPotIndex].textureId);
            }

            glPushMatrix();
            glRotatef(TotalRot, 0.f, 1.f, 0.f);
            glScalef(1.f, 1.f, 1.f);  // 0.1에서 0.5로 수정

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < potModels[currentPotIndex].faces.size(); i++) {
                unsigned int vertexIndex = potModels[currentPotIndex].faces[i];

                // 법선 벡터
                if (vertexIndex < potModels[currentPotIndex].normal.size()) {
                    glm::vec3 normal = potModels[currentPotIndex].normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
                }

                // 텍스처 좌표
                if (vertexIndex < potModels[currentPotIndex].texCoords.size()) {
                    glm::vec2 texCoord = potModels[currentPotIndex].texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                // 정점
                glm::vec3 vertex = potModels[currentPotIndex].vertices[vertexIndex];
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            glEnd();
            glPopMatrix();

            if (mat.hasTexture) {
                glDisable(GL_TEXTURE_2D);
            }
            glDisable(GL_NORMALIZE);  // 정규화 비활성화
        }
        
        // 임시 치 중인 식물 렌더링
        if (plantModels[currentPlantindex].isLoaded && plantmode) {
            // 투명 효과를 위한 블렌딩 활성화
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            Material& mat = plantModels[currentPlantindex].material;
            GLfloat ambient[] = { mat.ambient.x, mat.ambient.y, mat.ambient.z, 0.5f };  // alpha를 0.5로 설정
            GLfloat diffuse[] = { mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, 0.5f };  // alpha를 0.5로 설정
            GLfloat specular[] = { mat.specular.x, mat.specular.y, mat.specular.z, 0.5f };  // alpha를 0.5로 설정
            GLfloat shininess[] = { mat.shininess };

            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

            // 텍스처 설정
            if (mat.hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, plantModels[currentPlantindex].textureId);
            }

            glPushMatrix();
            float potHeight = getPotHeight(potModels[currentPotIndex]);
            glTranslatef(keyx, potHeight, keyz);
            glRotatef(0, 0.f, 1.f, 0.0f);
            
            glScalef(1.f, 1.f, 1.f);  // 0.1에서 0.5로 수정

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < plantModels[currentPlantindex].faces.size(); i++) {
                unsigned int vertexIndex = plantModels[currentPlantindex].faces[i];

                // 법선 벡터
                if (vertexIndex < plantModels[currentPlantindex].normal.size()) {
                    glm::vec3 normal = plantModels[currentPlantindex].normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
                }

                // 텍스처 좌표
                if (vertexIndex < plantModels[currentPlantindex].texCoords.size()) {
                    glm::vec2 texCoord = plantModels[currentPlantindex].texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                // 정점
                glm::vec3 vertex = plantModels[currentPlantindex].vertices[vertexIndex];
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            glEnd();
            glPopMatrix();

            if (mat.hasTexture) {
                glDisable(GL_TEXTURE_2D);
            }
            glDisable(GL_NORMALIZE);  // 정규화 비활성화

            glDisable(GL_BLEND);  // 블렌딩 비성화
        }
        
        // 정된 식물들 렌더링
        for (auto& plant : placedPlants) {
            if (!plant.isActive) continue;
            
            // 시간에 따른 크기 계산
            float growthTime = elapsed_time - plant.spawnTime;  // 식물이 란 시간
            float maxGrowthTime = 15.0f;  // 최대 성장까지 걸리는 시간 (15초)
            float growthProgress = std::min(growthTime / maxGrowthTime, 1.0f);  // 0~1 사이 값
            
            // 0.6에서 1.0까지 선형 보간
            plant.scale = 0.6f + (1.0f - 0.6f) * growthProgress;
            
            // 완전히 자랐을 때 자동 수확
            if (plant.scale >= 0.99f && plant.isActive) {
                plant.isActive = false;
                score += 100;
                continue;
            }

            const Material& mat = plant.Model.material;  // const 참조로 변경
            GLfloat ambient[] = { mat.ambient.x, mat.ambient.y, mat.ambient.z, 1.0f };  // alpha를 1.0으로 설정
            GLfloat diffuse[] = { mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, 1.0f };  // alpha를 1.0으로 설정
            GLfloat specular[] = { mat.specular.x, mat.specular.y, mat.specular.z, 1.0f };  // alpha를 1.0으로 설정
            GLfloat shininess[] = { mat.shininess };

            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

            if (mat.hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, plant.Model.textureId);
            }

            glPushMatrix();
            glTranslatef(plant.x, plant.y, plant.z);
            glRotatef(plant.rx, 1.0f, 0.0f, 0.0f);
            glRotatef(plant.ry, 0.0f, 1.0f, 0.0f);
            glRotatef(plant.rz, 0.0f, 0.0f, 1.0f);
            glScalef(1.0f * plant.scale, 1.0f * plant.scale, 1.0f * plant.scale);  // scale 적용

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < plant.Model.faces.size(); i++) {
                unsigned int vertexIndex = plant.Model.faces[i];

                // 법선 벡터
                if (vertexIndex < plant.Model.normal.size()) {
                    glm::vec3 normal = plant.Model.normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
                }

                // 텍스처 좌표
                if (vertexIndex < plant.Model.texCoords.size()) {
                    glm::vec2 texCoord = plant.Model.texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                // 정점
                glm::vec3 vertex = plant.Model.vertices[vertexIndex];
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            glEnd();
            glPopMatrix();

            if (mat.hasTexture) {
                glDisable(GL_TEXTURE_2D);
            }
        }
        
        // 2D 오버레이로 점수 표시
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 1280, 0, 720);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // 점수 텍스트 출력
        glColor3f(0.0f, 0.0f, 0.0f);  // 검정색으로 설정
        char scoreText[32];
        sprintf_s(scoreText, "Score: %d", score);
        glRasterPos2i(10, 10);  // 왼쪽 하단에 위치
        for (const char* c = scoreText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
        
        break;
    }

    // quadric 객체 정리를 switch문 밖에서
    if (quadric != nullptr) {
        gluDeleteQuadric(quadric);
    }

    glutSwapBuffers();
}

GLvoid Reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

void keyboard(unsigned char key, int x, int y) {
    float moveSpeed = 1.0f;  // 초당 이동 거리
    float deltaTime = 1.0f / 60.0f;  // 프레임당 시간 (60fps 기준)

    if (gameState == POT_SELECT) {
        switch (key) {
        case 27:  // ESC key
            gameState = MAIN_MENU;
            break;
        }
    }
    if (gameState == GAME_PLAY) {
        switch (key)
        {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            if (!plantmode) {
                plantmode = true;
                currentPlantindex = key - '1';  // 0-6 사이의 인덱스로 변환
                keyx = 0;
                keyz = 0;
            }
            else {
                // 현재 선택된 식물과 같은 키를 눌렀을 때만 plantmode 종료
                if (currentPlantindex == key - '1') {
                    plantmode = false;
                }
                else {
                    // 다른 숫자키를 눌렀을 경우 식물만 변경
                    currentPlantindex = key - '1';
                }
            }
            break;

        case '\r':  // Enter 키
            if (plantmode) {
                float potHeight = getPotHeight(potModels[currentPotIndex]);
                placePlant(currentPlantindex, keyx, potHeight, keyz);
                plantmode = false;
                keyx = 0;
                keyz = 0;
            }
            break;

        case 'w':
            if (plantmode) {
                float newZ = keyz - moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                // 화분 범위 체크
                if (sqrt(keyx*keyx + newZ*newZ) <= currentPotRadius) {
                    keyz = newZ;
                }
            }
            break;
        case 's':
            if (plantmode) {
                float newZ = keyz + moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                if (sqrt(keyx*keyx + newZ*newZ) <= currentPotRadius) {
                    keyz = newZ;
                }
            }
            break;
        case 'a':
            if (plantmode) {
                float newX = keyx - moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                if (sqrt(newX*newX + keyz*keyz) <= currentPotRadius) {
                    keyx = newX;
                }
            }
            break;
        case 'd':
            if (plantmode) {
                float newX = keyx + moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                if (sqrt(newX*newX + keyz*keyz) <= currentPotRadius) {
                    keyx = newX;
                }
            }
            break;
        default:
            break;
        }
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    int convertedY = 720 - y;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        switch (gameState) {
        case MAIN_MENU:
            if (x >= startButton.x && x <= startButton.x + startButton.width &&
                convertedY >= startButton.y && convertedY <= startButton.y + startButton.height) {
                gameState = POT_SELECT;
                glutPostRedisplay();
            }
            if (x >= exitButton.x && x <= exitButton.x + exitButton.width &&
                convertedY >= exitButton.y && convertedY <= exitButton.y + exitButton.height) {
                exit(0);
            }
            break;

        case POT_SELECT:
            if (x >= leftArrowButton.x && x <= leftArrowButton.x + leftArrowButton.width &&
                convertedY >= leftArrowButton.y && convertedY <= leftArrowButton.y + leftArrowButton.height) {
                currentPotIndex = (currentPotIndex - 1 + NUM_POTS) % NUM_POTS;
                glutPostRedisplay();
            }
            if (x >= rightArrowButton.x && x <= rightArrowButton.x + rightArrowButton.width &&
                convertedY >= rightArrowButton.y && convertedY <= rightArrowButton.y + rightArrowButton.height) {
                currentPotIndex = (currentPotIndex + 1) % NUM_POTS;
                glutPostRedisplay();
            }
            if (x >= confirmButton.x && x <= confirmButton.x + confirmButton.width &&
                convertedY >= confirmButton.y && convertedY <= confirmButton.y + confirmButton.height) {
                gameState = GAME_PLAY;
                glutPostRedisplay();
            }
            break;

        case GAME_PLAY:
            click = true;
            prex = x;
            prey = y;
            break;
        }
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        switch (gameState)
        {
        case MAIN_MENU:
            break;
        case POT_SELECT:
            break;
        case GAME_PLAY:
            click = false;
            break;
        default:
            break;
        }
    }
}