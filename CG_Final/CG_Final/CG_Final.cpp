// 표준 라이브러리
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <map>
#include <sstream>

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
float potRotation = 0.0f;  // 화분 회전 각도만 여기에 남김

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
    // 디버깅을 위한 출력 추가
    std::cout << "Loading texture: " << filename << std::endl;
    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!image) {
        std::cerr << "이미지 로드 실패: " << filename << std::endl;
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        return;
    }

    std::cout << "Texture loaded successfully: " << width << "x" << height 
              << " channels: " << channels << std::endl;

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);

    // 텍스처 파라미터 설정
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 텍스처 데이터 전송
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // 텍스처 ID 확인
    std::cout << "Generated texture ID: " << *texture << std::endl;

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

// 전역 변수 추가 (파일 상단에 추가)
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
            // 초당 90도 회전하도록 설정
            float rotationSpeed = 90.0f;
            float deltaTime = 1.0f / 60.0f;
            potRotation += rotationSpeed * deltaTime * ((prex > x) ? 1 : -1);
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



// 키보드 콜 추가
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

// Material 구조체 추복 선언 제거하고 하나로 통합
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    std::string diffuseTexPath;  // 텍스처 경로
    bool hasTexture;  // 텍스처 유무 확인을 위한 필드 추가
};

// Model 구조체 수정
struct Model {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> faces;
    Material material;
    bool isLoaded;
    GLuint textureId;
    float height;
    std::string mtlPath;  // MTL 파일 경로
    std::map<std::string, Material> materials;  // 여러 재질을 저장할 맵 추가
    std::string currentMaterialName;  // 현재 사용 중인 재질 이름
};

// Plant 구조체 수정
struct Plant {
    Model Model;
    float x, y, z;  // 위치
    float rx, ry, rz;  // 회전
    float spawnTime;  // 생성 시간
    bool isActive;  // 현재 배치된 상태인지
    float scale;  // 크기 비율
    bool isWatered;     // 물주기 상태 추가
    float waterTime;    // 물준 시간 저장
    int waterCount;     // 물주기 횟수 추가
    float savedScale;  // 물주기가 끝났�� 때의 크기를 저장
    bool isHarvesting;    // 수확 중인지 여부
    float harvestTime;    // 수확 시작 시간
};

// 전역 변수 추가
std::vector<Model> potModels;  // 화분 모델들을 저장할 벡터

std::vector<Model> plantModels;

// 전역 변수 추가
std::vector<Plant> placedPlants;  // 배치된 식물들을 저장할 벡터

// MTL 파일 로드 함수 개선
std::map<std::string, Material> LoadMTL(const char* filename) {
    std::map<std::string, Material> materials;
    std::ifstream file(filename);
    std::string line;
    Material* currentMaterial = nullptr;
    
    if (!file.is_open()) {
        std::cerr << "MTL 파일을 열 수 없습니다: " << filename << std::endl;
        return materials;
    }
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        
        if (token == "newmtl") {
            std::string matName;
            iss >> matName;
            materials[matName] = Material();
            currentMaterial = &materials[matName];
            currentMaterial->hasTexture = false;
            currentMaterial->shininess = 0.0f;
        }
        else if (currentMaterial != nullptr) {
            if (token == "Ka") {
                iss >> currentMaterial->ambient.x >> currentMaterial->ambient.y >> currentMaterial->ambient.z;
            }
            else if (token == "Kd") {
                iss >> currentMaterial->diffuse.x >> currentMaterial->diffuse.y >> currentMaterial->diffuse.z;
            }
            else if (token == "Ks") {
                iss >> currentMaterial->specular.x >> currentMaterial->specular.y >> currentMaterial->specular.z;
            }
            else if (token == "Ns") {
                iss >> currentMaterial->shininess;
            }
            else if (token == "map_Kd") {
                iss >> currentMaterial->diffuseTexPath;
                currentMaterial->hasTexture = true;
            }
        }
    }
    
    return materials;
}

// OBJ 파일 로드 함수 수정
void loadFromObjFile(const char* filename, Model& model) {
    FILE* file;
    char line[1000];
    float x, y, z;
    //unsigned int v[3], vt[3], vn[3];

    fopen_s(&file, filename, "r");
    if (!file) {
        std::cerr << "파일 열기 오류: " << filename << std::endl;
        return;
    }

    // 기본 데이터 초기화
    model.vertices.clear();
    model.faces.clear();
    model.normal.clear();
    model.texCoords.clear();

    // 임시 텍스처 좌표만 저장
    std::vector<glm::vec2> tempTexCoords;

    // 첫 번째 패스: 모든 버텍스와 텍스처 좌표 읽기
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            if (sscanf_s(line + 2, "%f %f %f", &x, &y, &z) == 3) {
                model.vertices.push_back(glm::vec3(x, y, -z));
                // 버텍스가 추가될 때마다 텍스처 좌표 공간도 미리 확보
                model.texCoords.push_back(glm::vec2(0, 0));
            }
        }
        else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
            if (sscanf_s(line + 3, "%f %f", &x, &y) == 2) {
                tempTexCoords.push_back(glm::vec2(x, y));  // Y좌표는 그대로 사용
            }
        }
    }

    // 이제 버텍스 수만큼 텍스처 좌표 벡터 크기 조정
    model.texCoords.resize(model.vertices.size());

    // 파일 포인터를 다시 처음으로
    rewind(file);

    // 두 번째 패스: face 정보 읽기
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == 'o' || strlen(line) == 0 || line[0] == 's')
            continue;

        if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
            if (sscanf_s(line + 3, "%f %f %f", &x, &y, &z) == 3) {
                model.normal.push_back(glm::vec3(x, y, -z));  // 원래 방식으로 노말 저장
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
                // 사각형 면 (4개�� 정점)
                numVertices = 4;
            }
            else if (sscanf_s(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u",
                &v[0], &vt[0], &vn[0],
                &v[1], &vt[1], &vn[1],
                &v[2], &vt[2], &vn[2]) == 9) {
                // 삼각형 면 (3개의 정점)
                numVertices = 3;
            }

            // 첫 번째 삼각형
            for (int i = 0; i < 3; i++) {
                unsigned int vertexIndex = v[i] - 1;
                model.faces.push_back(vertexIndex);
                
                if (!tempTexCoords.empty() && vt[i] > 0) {
                    // 해당 버텍스 인덱스에 텍스처 좌표 저장
                    model.texCoords[vertexIndex] = tempTexCoords[vt[i] - 1];
                }
            }

            // 사각형 째 삼 같은 방식으로
            if (numVertices == 4) {
                for (int i = 0; i < 3; i++) {
                    unsigned int vertexIndex;
                    unsigned int texCoordIndex;
                    
                    switch(i) {
                        case 0: vertexIndex = v[2] - 1; texCoordIndex = vt[2] - 1; break;
                        case 1: vertexIndex = v[3] - 1; texCoordIndex = vt[3] - 1; break;
                        case 2: vertexIndex = v[0] - 1; texCoordIndex = vt[0] - 1; break;
                    }
                    
                    model.faces.push_back(vertexIndex);
                    if (!tempTexCoords.empty()) {
                        model.texCoords[vertexIndex] = tempTexCoords[texCoordIndex];
                    }
                }
            }
        }
        else if (strncmp(line, "mtllib", 6) == 0) {
            char mtlPath[256];
            char fullPath[512];
            sscanf_s(line + 7, "%s", mtlPath, (unsigned)sizeof(mtlPath));
            sprintf_s(fullPath, "Models/%s", mtlPath);
            model.mtlPath = fullPath;
            
            // MTL 파일에서 모든 재질 로드하여 모델의 materials 맵에 저장
            model.materials = LoadMTL(fullPath);
            
            // 첫 번 질 기본으로 정
            if (!model.materials.empty()) {
                auto firstMaterial = model.materials.begin();
                model.material = firstMaterial->second;
                model.currentMaterialName = firstMaterial->first;
            }
        }
        else if (strncmp(line, "usemtl", 6) == 0) {
            char matName[256];
            sscanf_s(line + 7, "%s", matName, (unsigned)sizeof(matName));
            
            // 모델 내부에 저장된 재 맵에서 찾아서 설정
            if (model.materials.find(matName) != model.materials.end()) {
                model.material = model.materials[matName];
                model.currentMaterialName = matName;
            }
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

// 충돌 체크를 위한 구조체 추가
struct BoundingCircle {
    float x, z;  // 중심점
    float radius;  // 반지름
};

// 식물의 충돌 영역 계산
BoundingCircle getPlantBounds(const Plant& plant) {
    BoundingCircle bounds;
    bounds.x = plant.x;
    bounds.z = plant.z;
    bounds.radius = 0.2f * plant.scale;  // 식물 크기에 따라 충돌 영역 조정
    return bounds;
}

// 두 원형 충돌 영역 간 충돌 검사
bool checkCircleCollision(const BoundingCircle& a, const BoundingCircle& b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    float distance = sqrt(dx*dx + dz*dz);
    return distance < (a.radius + b.radius);
}

// getPotRadius 함수 추가
float getPotRadius(const Model& potModel) {
    if (!potModel.isLoaded || potModel.vertices.empty()) {
        return 0.5f;  // 기본
    }

    float maxRadius = 0.0f;
    for (const auto& vertex : potModel.vertices) {
        float radius = sqrt(vertex.x * vertex.x + vertex.z * vertex.z);
        maxRadius = std::max(maxRadius, radius);
    }

    return maxRadius;
}

// 식물 배치 가능 여부 확인
bool canPlacePlant(float x, float z, float scale) {
    BoundingCircle newPlant = {x, z, 0.2f * scale};  // 새로운 식물의 충돌 영역
    
    // 기존 식물들과의 충돌 검사
    for (const auto& plant : placedPlants) {
        if (!plant.isActive) continue;
        
        BoundingCircle existingPlant = getPlantBounds(plant);
        if (checkCircleCollision(newPlant, existingPlant)) {
            return false;  // 충돌 발생
        }
    }
    
    // 화분 경계 체크
    float potRadius = getPotRadius(potModels[currentPotIndex]);
    if (sqrt(x*x + z*z) > potRadius - newPlant.radius) {
        return false;  // 화분 경계 체크
    }
    
    return true;  // 배치 가능
}

// placePlant 함수 수정
void placePlant(int plantIndex, float x, float y, float z) {
    // 배치 가능 여부 확인
    if (!canPlacePlant(x, z, 0.6f)) {  // 초기 스케일 0.6f 사용
        std::cout << "Cannot place plant here!" << std::endl;
        return;
    }

    Plant newPlant;
    newPlant.Model = plantModels[plantIndex];
    newPlant.x = x;
    newPlant.y = y - 0.1f;  // 흙 높이에 맞춤
    newPlant.z = z;
    newPlant.rx = 0;
    newPlant.ry = 0;
    newPlant.rz = 0;
    newPlant.spawnTime = elapsed_time;
    newPlant.isActive = true;
    newPlant.scale = 0.6f;
    newPlant.savedScale = 0.6f;  // 초기 크기 저장
    newPlant.isWatered = false;  // 초기에는 물주기 상태가 아님
    newPlant.waterTime = 0.0f;
    newPlant.waterCount = 0;  // 물주기 횟수 초기화
    newPlant.isHarvesting = false;
    newPlant.harvestTime = 0.0f;
    
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
    return height > 0 ? height : 0.5f;  // 이가 0 하면 기본 반환
}

// 전역 변수 추가
int score = 0;  // 점수 저장 변수

// 그림자 맵핑을 위한 전역 변수 추가
GLfloat global_light_position[] = { 5.0f, 5.0f, 5.0f, 1.0f };

// 전역 변수 추가
static bool wateringMode = false;  // 물주기 모드 상태
static double waterX = 0, waterZ = 0;  // 물 위치

// 식물이 물주기 영역 안에 있는지 확인하는 함수 추가
bool isPlantInWateringArea(const Plant& plant, float waterX, float waterZ, float waterRadius) {
    float dx = plant.x - waterX;
    float dz = plant.z - waterZ;
    float distance = sqrt(dx*dx + dz*dz);
    return distance < waterRadius;
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
    std::cout << "Loading textures..." << std::endl;

    // 메인 타이틀 스처
    loadTexture("Textures/MainTitle.png", &startPageTexture);
    std::cout << "Main title texture ID: " << startPageTexture << std::endl;

    // 화분 텍스처
    loadTexture("Models/Pot_01.png", &potModels[0].textureId);
    std::cout << "Pot 1 texture ID: " << potModels[0].textureId << std::endl;
    potModels[0].material.hasTexture = true;  // hasTexture 플래그 설정

    loadTexture("Models/Pot_02.png", &potModels[1].textureId);
    std::cout << "Pot 2 texture ID: " << potModels[1].textureId << std::endl;
    potModels[1].material.hasTexture = true;  // hasTexture 플래그 설정

    plantModels.resize(NUM_PLANTS);
    loadFromObjFile("Models/Cactus_01.obj", plantModels[0]);
    loadFromObjFile("Models/Flower_01.obj", plantModels[1]);
    loadFromObjFile("Models/Flower_02.obj", plantModels[2]);
    loadFromObjFile("Models/Grass_01.obj", plantModels[3]);
    loadFromObjFile("Models/Grass_02.obj", plantModels[4]);
    loadFromObjFile("Models/Tree_01.obj", plantModels[5]);
    loadFromObjFile("Models/Tree_01.obj", plantModels[6]);

    loadTexture("Models/Cactus_01.png", &plantModels[0].textureId);
    plantModels[0].material.hasTexture = true;
    loadTexture("Models/Flower_01.png", &plantModels[1].textureId);
    plantModels[1].material.hasTexture = true;
    loadTexture("Models/Flower_03.png", &plantModels[2].textureId);
    plantModels[2].material.hasTexture = true;
    loadTexture("Models/Grass_01.png", &plantModels[3].textureId);
    plantModels[3].material.hasTexture = true;
    loadTexture("Models/Grass_02.png", &plantModels[4].textureId);
    plantModels[4].material.hasTexture = true;
    loadTexture("Models/Tree_01.png", &plantModels[5].textureId);
    plantModels[5].material.hasTexture = true;
    loadTexture("Models/Tree_02.png", &plantModels[6].textureId);
    plantModels[6].material.hasTexture = true;

    // 시 시간 설정
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
    // 배경색을 밝 회색으로 변경
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 수 언을 switch문 앞으로 이동
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
        // 렌더링 드 설정 추가
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 와이어프레임이 아닌 채워진 폴리곤으로 렌더링

        // 법선 터 동 정규화 활���
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

            // 텍스처 설정을 재�� 설정 전에 수행
            if (mat.hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, potModels[currentPotIndex].textureId);
                
                // 디버깅 출력 제거
                // GLint currentTexture;
                // glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
                // std::cout << "Current bound texture: " << currentTexture << std::endl;
                
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                
                // 텍스처 파라미터 설정
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }

            // 재질 적용
            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

            glPushMatrix();
            glRotatef(30, 0, 1, 0);
            glScalef(0.5f, 0.5f, 0.5f);  // 0.1에서 0.5로 수정

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < potModels[currentPotIndex].faces.size(); i++) {
                unsigned int vertexIndex = potModels[currentPotIndex].faces[i];

                // 렌더링 시 텍스처 좌표 적용
                if (mat.hasTexture && vertexIndex < potModels[currentPotIndex].texCoords.size()) {
                    glm::vec2 texCoord = potModels[currentPotIndex].texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                // 법선 벡터
                if (vertexIndex < potModels[currentPotIndex].normal.size()) {
                    glm::vec3 normal = potModels[currentPotIndex].normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
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
        // 게임 플레이 렌더링 �� 머지 현
        // 렌더링 모 설 추가
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 이어프레임이 아닌 채워진 폴리곤으로 렌더링

        // 법선 벡터 자동 정규화 활성화
        glEnable(GL_NORMALIZE);

        // 조명 설정 
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        // 조명 설정 적용 (변수 선언 제거)
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

        // 감쇠 설정 (선택사항)
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);

        // 3D 투�� 설정
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)1280 / 720, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // 카메라 위치 계산
        float camX = cameraDistance * sin(cameraAngle * 3.14159f / 180.0f);
        float camZ = cameraDistance * cos(cameraAngle * 3.14159f / 180.0f);
        float camY = cameraDistance * 1.2f;  // 카메라를 더 높게 위치시킴

        // 보는 지점의 Y값 조정
        float lookAtY = -0.5f;  // 고정된 Y값으로 설정

        // 카메라 뒤에 조명 위치 설정
        light_position[0] = camX * 1.2f;  // 존 배열 재사용
        light_position[1] = camY * 1.1f;
        light_position[2] = camZ * 1.2f;
        light_position[3] = 1.0f;

        // 조명 설정 적용
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);

        // 카메라 설정
        gluLookAt(camX, camY, camZ,
                  0.0f, lookAtY, 0.0f,
                  0.0f, 1.0f, 0.0f);

        if (potModels[currentPotIndex].isLoaded) {
            // 재질 설정
          
            Material& mat = potModels[currentPotIndex].material;
            GLfloat ambient[] = { mat.ambient.x, mat.ambient.y, mat.ambient.z, 1.0f };
            GLfloat diffuse[] = { mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, 1.0f };
            GLfloat specular[] = { mat.specular.x, mat.specular.y, mat.specular.z, 1.0f };
            GLfloat shininess[] = { mat.shininess };

            // 텍스처 설정을 재질 설정 전에 수행
            if (mat.hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, potModels[currentPotIndex].textureId);
                
                // 버깅 출력 제거
                // GLint currentTexture;
                // glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
                // std::cout << "Current bound texture: " << currentTexture << std::endl;
                
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                
                // 텍스처 파라미터 설정
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }

            // 재질 적용
            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

            glPushMatrix();
            glTranslatef(0.0f, -0.5f, 1.0f);      // 먼저 위치 이동
            glRotatef(potRotation, 0.f, 1.f, 0.f); // 그 다음 회전 (화���의 중심을 기준으로)
            glScalef(1.f, 1.f, 1.0f);

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < potModels[currentPotIndex].faces.size(); i++) {
                unsigned int vertexIndex = potModels[currentPotIndex].faces[i];

                // 렌더링 시 텍스처 좌표 적용
                if (mat.hasTexture && vertexIndex < potModels[currentPotIndex].texCoords.size()) {
                    glm::vec2 texCoord = potModels[currentPotIndex].texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                // 법선 벡터
                if (vertexIndex < potModels[currentPotIndex].normal.size()) {
                    glm::vec3 normal = potModels[currentPotIndex].normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
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
        }
        
        // 임시 식물 렌더링 (plantmode)
        if (plantModels[currentPlantindex].isLoaded && plantmode) {
            // ��명 효과를 위 블렌딩 활성화
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            Material& mat = plantModels[currentPlantindex].material;
            GLfloat ambient[] = { mat.ambient.x, mat.ambient.y, mat.ambient.z, 0.5f };  // alpha를 0.5로 설정
            GLfloat diffuse[] = { mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, 0.5f };  // alpha를 0.5 설정
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
            glTranslatef(0.0f, -0.5f, 1.0f);      // 화분과 같은 위치로 이동
            glRotatef(potRotation, 0.f, 1.f, 0.f); // 화분과 같이 회전
            glTranslatef(keyx, potHeight - 0.1f, keyz);   // 흙 높이에 맞춤
            glRotatef(0, 0.f, 1.f, 0.0f);
            
            glScalef(1.f, 1.f, 1.f);  // 0.1에서 0.5로 수정

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < plantModels[currentPlantindex].faces.size(); i++) {
                unsigned int vertexIndex = plantModels[currentPlantindex].faces[i];

                // 렌더링 시 텍스처 좌 적용
                if (mat.hasTexture && vertexIndex < plantModels[currentPlantindex].texCoords.size()) {
                    glm::vec2 texCoord = plantModels[currentPlantindex].texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                // 법선 벡터
                if (vertexIndex < plantModels[currentPlantindex].normal.size()) {
                    glm::vec3 normal = plantModels[currentPlantindex].normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
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
        
        // 물주기 모드 렌더링
        if (wateringMode) {
            // 화분 높이 계산
            float potHeight = getPotHeight(potModels[currentPotIndex]);

            glPushMatrix();
            glTranslatef(0.0f, -0.5f, 1.0f);
            glRotatef(potRotation, 0.0f, 1.0f, 0.0f);
            
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            glColor4f(0.0f, 0.4f, 1.0f, 0.6f);
            glTranslatef(waterX, potHeight - 0.09f, waterZ);

            // 원형 디스크 그리기
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0f, 0.0f, 0.0f);  // 중심점
            const float waterRadius = 0.2f;
            const int waterSegments = 32;
            for (int i = 0; i <= waterSegments; i++) {
                float angle = (float)i / waterSegments * 2.0f * 3.14159f;
                float x = cos(angle) * waterRadius;
                float z = sin(angle) * waterRadius;
                glVertex3f(x, 0.0f, z);
            }
            glEnd();

            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
            glEnable(GL_TEXTURE_2D);
            glPopMatrix();
        }
        
        // 배치된 식물들 렌더링
        for (auto& plant : placedPlants) {
            if (!plant.isActive) continue;

            // 수확 중인 식물 처리
            if (plant.isHarvesting) {
                float harvestDuration = elapsed_time - plant.harvestTime;
                if (harvestDuration >= 3.0f) {  // 3초 후에 사라짐
                    plant.isActive = false;
                    score += 100;
                    continue;
                }
                // 노란색으로 변하는 효과
                const Material& mat = plant.Model.material;
                GLfloat yellow[4] = { 1.0f, 1.0f, 0.0f, 1.0f };  // 노란색
                GLfloat ambient[4], diffuse[4], specular[4];
                
                // 원래 색상과 노란색 사이를 보간 (3초에 걸쳐서)
                float t = harvestDuration / 3.0f;  // 0~1 사이 값
                for (int i = 0; i < 3; i++) {
                    ambient[i] = (1-t) * mat.ambient[i] + t * yellow[i];
                    diffuse[i] = (1-t) * mat.diffuse[i] + t * yellow[i];
                    specular[i] = (1-t) * mat.specular[i] + t * yellow[i];
                }
                ambient[3] = diffuse[3] = specular[3] = 1.0f;
                
                glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
                glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
                glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            }
            else if (plant.isWatered) {
                float waterDuration = elapsed_time - plant.waterTime;
                if (waterDuration > 5.0f) {  // 5초가 지나면
                    plant.isWatered = false;  // 물주기 상태 해제
                    plant.savedScale = plant.scale;  // 현재 크기 저장
                }

                // 물주기 상태일 때만 성장
                float growthPerWater = (1.0f - 0.6f) / 3.0f;  // 전체 성장을 3단계로 나눔
                float minScale = plant.savedScale;  // 저장된 크기부터 시작
                float maxScale = 0.6f + (growthPerWater * (plant.waterCount + 1));  // 다음 단계까지
                
                float stageProgress = waterDuration / 5.0f;  // 현재 물주기 단계에서의 진행도
                plant.scale = minScale + (maxScale - minScale) * stageProgress;

                if (waterDuration >= 5.0f) {
                    plant.waterCount++;
                    plant.savedScale = plant.scale;  // 현재 크기 저장
                }
            }

            const Material& mat = plant.Model.material;
            GLfloat ambient[4], diffuse[4], specular[4];

            if (plant.isWatered) {  // 물을 준 상태일 때
                ambient[0] = mat.ambient.x * 0.7f;
                ambient[1] = mat.ambient.y * 0.7f;
                ambient[2] = mat.ambient.z * 0.7f;
                ambient[3] = 1.0f;

                diffuse[0] = mat.diffuse.x * 0.7f;
                diffuse[1] = mat.diffuse.y * 0.7f;
                diffuse[2] = mat.diffuse.z * 0.7f;
                diffuse[3] = 1.0f;

                specular[0] = 1.0f;
                specular[1] = 1.0f;
                specular[2] = 1.0f;
                specular[3] = 1.0f;

                GLfloat shininess[] = { mat.shininess * 2.0f };
                glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
            }
            else {
                ambient[0] = mat.ambient.x;
                ambient[1] = mat.ambient.y;
                ambient[2] = mat.ambient.z;
                ambient[3] = 1.0f;

                diffuse[0] = mat.diffuse.x;
                diffuse[1] = mat.diffuse.y;
                diffuse[2] = mat.diffuse.z;
                diffuse[3] = 1.0f;

                specular[0] = mat.specular.x;
                specular[1] = mat.specular.y;
                specular[2] = mat.specular.z;
                specular[3] = 1.0f;

                GLfloat shininess[] = { mat.shininess };
                glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
            }

            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

            // 텍스처 설정
            if (mat.hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, plant.Model.textureId);
            }

            glPushMatrix();
            glTranslatef(0.0f, -0.5f, 1.0f);
            glRotatef(potRotation, 0.f, 1.f, 0.f);
            glTranslatef(plant.x, plant.y, plant.z);
            glRotatef(plant.rx, 1.0f, 0.0f, 0.0f);
            glRotatef(plant.ry, 0.0f, 1.0f, 0.0f);
            glRotatef(plant.rz, 0.0f, 0.0f, 1.0f);
            glScalef(plant.scale, plant.scale, plant.scale);

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < plant.Model.faces.size(); i++) {
                unsigned int vertexIndex = plant.Model.faces[i];

                if (mat.hasTexture && vertexIndex < plant.Model.texCoords.size()) {
                    glm::vec2 texCoord = plant.Model.texCoords[vertexIndex];
                    glTexCoord2f(texCoord.x, texCoord.y);
                }

                if (vertexIndex < plant.Model.normal.size()) {
                    glm::vec3 normal = plant.Model.normal[vertexIndex];
                    glNormal3f(normal.x, normal.y, normal.z);
                }

                glm::vec3 vertex = plant.Model.vertices[vertexIndex];
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            glEnd();
            glPopMatrix();

            if (mat.hasTexture) {
                glDisable(GL_TEXTURE_2D);
            }
        }
        
        // 바닥 평면과 그림자 렌더링
        float potTopHeight = getPotHeight(potModels[currentPotIndex]);
        float potRadius = getPotRadius(potModels[currentPotIndex]) * 0.97f;

        // 바닥 평면 (화분 윗면)
        glPushMatrix();
        glTranslatef(0.0f, -0.5f, 1.0f);
        glRotatef(potRotation, 0.0f, 1.0f, 0.0f);

        // 깊이 버퍼 설정 수정
        glDepthMask(GL_TRUE);  // 깊이 버퍼 쓰기 활성화
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        // 원형 바닥면 그리기 - 조명과 텍스처 비활성화
        glColor4f(0.25f, 0.15f, 0.10f, 1.0f);  // 더 어두운 흙색
        glTranslatef(0.0f, potTopHeight - 0.1f, 0.0f);

        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, 0.0f);  // 중심점
        const int segments = 32;
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            float x = cos(angle) * potRadius;
            float z = sin(angle) * potRadius;
            glVertex3f(x, 0.0f, z);
        }
        glEnd();

        // 그림자 효과
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // 그림자는 깊이 버퍼에 쓰지 않음

        // 식물들의 그림자
        for (const auto& plant : placedPlants) {
            if (!plant.isActive) continue;
            
            float distFromCenter = sqrt(plant.x * plant.x + plant.z * plant.z);
            if (distFromCenter > potRadius * 0.9f) continue;

            glPushMatrix();
            glTranslatef(plant.x, 0.005f, plant.z);
            glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
            glScalef(1.0f * plant.scale, 0.01f, 1.0f * plant.scale);  // 0.55f에서 1.0f로 수정

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < plant.Model.faces.size(); i++) {
                unsigned int vertexIndex = plant.Model.faces[i];
                glm::vec3 vertex = plant.Model.vertices[vertexIndex];
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            glEnd();
            glPopMatrix();
        }

        glDepthMask(GL_TRUE);  // 깊이 버퍼 쓰기 다시 활성화
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);  // 조명 다시 활성화
        glEnable(GL_TEXTURE_2D);  // 텍스처 다시 활성화
        glPopMatrix();

        // 2D 오버레이로 점수 표시
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 1280, 0, 720);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // 점수 텍스트 출력
        glColor3f(0.0f, 0.0f, 0.0f);  // 검정으로 설정
        char scoreText[32];
        sprintf_s(scoreText, "Score: %d", score);
        glRasterPos2i(10, 10);  // 왼쪽 하단에 치
        for (const char* c = scoreText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
        
        break;
    }

    // quadric 객체 정리 switch문 밖에서
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
        case '1': case '2': case '3': case '4': case '5': case '6': case '7':
            if (wateringMode) {
                wateringMode = false;  // 물주기 모드 종료
                plantmode = true;      // 식물 모드 시작
                keyx = waterX;         // 현재 위치 유지
                keyz = waterZ;
                currentPlantindex = key - '1';
            }
            else if (plantmode) {
                if (currentPlantindex == key - '1') {  // 같은 숫자를 누르면
                    plantmode = false;  // 식물 모드 종료
                    keyx = 0;
                    keyz = 0;
                } else {  // 다른 숫자를 누르면
                    currentPlantindex = key - '1';  // 식물만 변경
                }
            }
            else {  // 아무 모드도 아닐 때
                plantmode = true;
                currentPlantindex = key - '1';
                keyx = 0;
                keyz = 0;
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
            else if (wateringMode) {
                for (auto& plant : placedPlants) {
                    if (plant.isActive && isPlantInWateringArea(plant, waterX, waterZ, 0.2f)) {
                        if (plant.waterCount >= 3 && plant.scale >= 0.99f) {  // 완전히 자란 식물이면
                            plant.isHarvesting = true;  // 수확 시작
                            plant.harvestTime = elapsed_time;  // 수확 시작 시간 기록
                            score += 100;
                            std::cout << "Plant harvested! Score: " << score << std::endl;
                        }
                        else if (!plant.isWatered && plant.waterCount < 3) {  // 아직 자라는 중이면 물주기
                            plant.isWatered = true;
                            plant.waterTime = elapsed_time;
                            std::cout << "Watering plant! Count: " << plant.waterCount + 1 << std::endl;
                        }
                    }
                }
            }
            break;

        case 'w':
            if (plantmode || wateringMode) {  // 식물 심기 모드나 물주기 모드일 때
                float newZ = (plantmode ? keyz : waterZ) - moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                float checkX = plantmode ? keyx : waterX;
                if (sqrt(checkX*checkX + newZ*newZ) <= currentPotRadius) {
                    if (plantmode) keyz = newZ;
                    else waterZ = newZ;
                }
            }
            break;
        case 's':
            if (plantmode || wateringMode) {
                float newZ = (plantmode ? keyz : waterZ) + moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                float checkX = plantmode ? keyx : waterX;
                if (sqrt(checkX*checkX + newZ*newZ) <= currentPotRadius) {
                    if (plantmode) keyz = newZ;
                    else waterZ = newZ;
                }
            }
            break;
        case 'a':
            if (plantmode || wateringMode) {
                float newX = (plantmode ? keyx : waterX) - moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                float checkZ = plantmode ? keyz : waterZ;
                if (sqrt(newX*newX + checkZ*checkZ) <= currentPotRadius) {
                    if (plantmode) keyx = newX;
                    else waterX = newX;
                }
            }
            break;
        case 'd':
            if (plantmode || wateringMode) {
                float newX = (plantmode ? keyx : waterX) + moveSpeed * deltaTime;
                float currentPotRadius = getPotRadius(potModels[currentPotIndex]);
                float checkZ = plantmode ? keyz : waterZ;
                if (sqrt(newX*newX + checkZ*checkZ) <= currentPotRadius) {
                    if (plantmode) keyx = newX;
                    else waterX = newX;
                }
            }
            break;
        case 'r':
        case 'R':
            if (plantmode) {  // 식물 모드일 때
                plantmode = false;
                wateringMode = true;
                waterX = keyx;  // 현재 위치 유지
                waterZ = keyz;
            } else if (wateringMode) {  // 물주기 모드일 때
                wateringMode = false;
                plantmode = true;
                keyx = waterX;  // 현재 위치 유지
                keyz = waterZ;
            } else {  // 둘 다 때
                wateringMode = true;
                waterX = 0;
                waterZ = 0;
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