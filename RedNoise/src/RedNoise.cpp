#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <Colour.h>
#include <ModelTriangle.h>
#include <TextureMap.h>
#include <Utils.h>
#include <map>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include<unistd.h>
#include "RayTriangleIntersection.h"
//std::cout << glm::to_string(myVector)

#define WIDTH 320*2
#define HEIGHT 240*2
#define IMAGEPLANE 240

// 0 for a wireframe scene, 1 for a rasterised scene, 2 for a raytraced scene
int renderMode = 2;
bool isSphere = false;
bool loadClosedCornellBox = false;
bool changeToPhong = true;

// DO NOT CHANGE BELOW THE LINE
//!!!!!!!!!!!!!!!!!!!!!!
bool paused = true;
bool loadSphere = false;

std::map<std::string, std::pair<glm::vec3, int>> vertexNormalsMap;
std::vector<std::vector<uint32_t>> textureArray;
std::map<std::string, Colour> colourPaletteMap;
std::vector<ModelTriangle> faces;
float depthsArray[WIDTH+1][HEIGHT+1] = {};
glm::vec3 cameraPosition = glm::vec3(0, 0, 4);
glm::vec3 initialCameraPosition = glm::vec3(0, 0, 4);
glm::vec3 lookAtGlobal = glm::vec3(0, 0, 0);
float refractiveIndexGlobal = 1.3f;
glm::vec3 lightSourcePosition = glm::vec3(0.5,0.0,1.2);


float angleX = 0;
float angleY = 0;
glm::mat3 rotationX = glm::mat3(
        1.0, 0.0, 0.0,
        0.0, cos(angleY), -sin(angleY),
        0.0, sin(angleY), cos(angleY)
);
glm::mat3 rotationY = glm::mat3(
        cos(angleX), 0.0, sin(angleX),
        0.0, 1.0, 0.0,
        -sin(angleX), 0.0, cos(angleX)
);

glm::mat3 positionY = glm::mat3(
        cos(angleX), 0.0, sin(angleX),
        0.0, 1.0, 0.0,
        -sin(angleX), 0.0, cos(angleX)
);

float orientationAngleX = 0;
float orientationAngleY = 0;
glm::mat3 cameraOrientation;
glm::mat3 orientationX = glm::mat3(
        1.0, 0.0, 0.0,
        0.0, cos(orientationAngleY), -sin(orientationAngleY),
        0.0, sin(orientationAngleY), cos(orientationAngleY)
);
glm::mat3 orientationY = glm::mat3(
        cos(orientationAngleX), 0.0, sin(orientationAngleX),
        0.0, 1.0, 0.0,
        -sin(orientationAngleX), 0.0, cos(orientationAngleX)
);


void setOrientationAngle(char axis, float angle) {
    if (axis == 'y') {
        orientationAngleY = orientationAngleY + angle;
        orientationY = glm::mat3(
                cos(orientationAngleY), 0.0, sin(orientationAngleY),
                0.0, 1.0, 0.0,
                -sin(orientationAngleY), 0.0, cos(orientationAngleY)
        );
    } else if (axis == 'x') {
        orientationAngleX = orientationAngleX + angle;
        orientationX = glm::mat3(
                1.0, 0.0, 0.0,
                0.0, cos(orientationAngleX), -sin(orientationAngleX),
                0.0, sin(orientationAngleX), cos(orientationAngleX)
        );
    }
    cameraOrientation = orientationY * orientationX;
}


void setRotationAngle(char axis, float angle) {
    if (axis == 'y') {
        angleY = angleY + angle;
        rotationY = glm::mat3(
                cos(angleY), 0.0, sin(angleY),
                0.0, 1.0, 0.0,
                -sin(angleY), 0.0, cos(angleY)
        );
    } else if (axis == 'x') {
        angleX = angleX + angle;
        rotationX = glm::mat3(
                1.0, 0.0, 0.0,
                0.0, cos(angleX), -sin(angleX),
                0.0, sin(angleX), cos(angleX)
        );
    }
}


std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
    std::vector<float> array;

    float increment = (to-from)/(numberOfValues-1);

    array.push_back(from);
    float prev = from;

    for (int i=1; i<numberOfValues; i++) {
        array.push_back(prev+increment);
        prev += increment;
    }

    return array;
}

std::vector<TexturePoint> interpolate2DPoints(TexturePoint from, TexturePoint to, int numberOfValues) {
    std::vector<TexturePoint> array;

    float incrementX = (to.x - from.x) / (numberOfValues - 1);
    float incrementY = (to.y - from.y) / (numberOfValues - 1);

    for (int i = 0; i < numberOfValues; i++) {
        float interpolatedX = from.x + incrementX * i;
        float interpolatedY = from.y + incrementY * i;
        array.push_back({interpolatedX, interpolatedY});
    }

    return array;
}


std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues) {
    std::vector<glm::vec3> array;

    float increment0 = (to[0]-from[0])/(numberOfValues-1);
    float increment1 = (to[1]-from[1])/(numberOfValues-1);
    float increment2 = (to[2]-from[2])/(numberOfValues-1);

    array.push_back(from);

    float prev0 = from[0];
    float prev1 = from[1];
    float prev2 = from[2];

    for (int i=1; i<numberOfValues; i++) {
        array.push_back(glm::vec3(prev0 + increment0, prev1 + increment1, prev2 + increment2));
        prev0 += increment0;
        prev1 += increment1;
        prev2 += increment2;
    }

    return array;
}

void draw2DLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour_class) {

    bool hasDepth = false;
    if (from.depth) {
        hasDepth = true;
    }

    bool hasTexture = false;
    if (from.texturePoint.x!=-1) {
        hasTexture = true;
    }

    float toX = to.x;
    float toY = to.y;
    float fromX = from.x;
    float fromY = from.y;

    float xDiff = toX - fromX;
    float yDiff = toY - fromY;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    float xStepSize = xDiff / numberOfSteps;
    float yStepSize = yDiff / numberOfSteps;

    std::vector<float> depths;
    if (hasDepth) {
        depths = interpolateSingleFloats(from.depth, to.depth, numberOfSteps+1);
    }

    std::vector<TexturePoint> textures;
    if (hasTexture) {
        textures = interpolate2DPoints(from.texturePoint, to.texturePoint, numberOfSteps+1);
    }

    if (numberOfSteps!=0) {

        for (float i = 0.0; i <= numberOfSteps; i++) {

            float x = fromX + (xStepSize * i);
            float y = fromY + (yStepSize * i);

            float red = colour_class.red;
            float green = colour_class.green;
            float blue = colour_class.blue;
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);

            if (hasDepth && x>=0 && y>=0 && x<WIDTH && y<HEIGHT && hasTexture) {
                float depth = depths[i];

                if (depth >= depthsArray[(int) floor(x)][(int) floor(y)] ) {
                    depthsArray[(int) floor(x)][(int) floor(y)] = depth;
                    uint32_t textureVar = textureArray[(int) floor(textures[i].x)][(int) floor(textures[i].y)];
                    window.setPixelColour(floor(x), floor(y), textureVar);
                }
            } else if (hasDepth && x>=0 && y>=0 && x<WIDTH && y<HEIGHT) {
                float depth = depths[i];

                if (depth >= depthsArray[(int) floor(x)][(int) floor(y)] ) {
                    depthsArray[(int) floor(x)][(int) floor(y)] = depth;
                    window.setPixelColour(floor(x), floor(y), colour);
                }
            } else if (hasTexture) {
                uint32_t textureVar = textureArray[(int) floor(textures[i].x)][(int) floor(textures[i].y)];
                window.setPixelColour(floor(x), floor(y), textureVar);
            } else {
                window.setPixelColour(floor(x), floor(y), colour);
            }
        }
    }
}


std::vector<CanvasPoint> Array_2DLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to) {

    bool hasDepth = false;
    if (from.depth) {
        hasDepth = true;
    }

    std::vector<CanvasPoint> result;

    float toX = to.x;
    float toY = to.y;
    float fromX = from.x;
    float fromY = from.y;

    float xDiff = toX - fromX;
    float yDiff = toY - fromY;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    float xStepSize = xDiff / numberOfSteps;
    float yStepSize = yDiff / numberOfSteps;

    std::vector<float> depths;
    if (hasDepth) {
        depths = interpolateSingleFloats(from.depth, to.depth, numberOfSteps+1);
    }

    for (float i = 0.0; i <= numberOfSteps; i++) {
        float x = fromX + (xStepSize * i);
        float y = fromY + (yStepSize * i);

        if (hasDepth) {
            float depth = depths[i];
            result.push_back(CanvasPoint(x, y, depth));
        } else {
            result.push_back(CanvasPoint(x, y));
        }

    }

    return result;
}


void drawTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour_class) {

    draw2DLine(window, triangle[0], triangle[1], colour_class);
    draw2DLine(window, triangle[1], triangle[2], colour_class);
    draw2DLine(window, triangle[2], triangle[0], colour_class);
}


void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour_class) {

    bool hasDepth = false;
    bool hasTexture = false;

    if (triangle[0].depth) {
        hasDepth = true;
    }

    if (triangle[0].texturePoint.x!=-1) {
        std::cout << "has texture" << std::endl;
        std::cout << triangle[0].texturePoint << std::endl;
        hasTexture = true;
    }

    CanvasPoint p0 = CanvasPoint(triangle[0].x, triangle[0].y);
    CanvasPoint pL = CanvasPoint(triangle[1].x, triangle[1].y);
    CanvasPoint p2 = CanvasPoint(triangle[2].x, triangle[2].y);

    if (hasDepth) {
        p0.depth = triangle[0].depth;
        pL.depth = triangle[1].depth;
        p2.depth = triangle[2].depth;
    }

    if (hasTexture) {
        p0.texturePoint = triangle[0].texturePoint;
        pL.texturePoint = triangle[1].texturePoint;
        p2.texturePoint = triangle[2].texturePoint;
    }

    // sort the points from top to bottom
    if (p0.y > p2.y) std::swap(p0, p2);
    if (p0.y > pL.y) std::swap(p0, pL);
    if (pL.y > p2.y) std::swap(pL, p2);

    float pR_x = p0.x - ((p2.x-p0.x)*(p0.y-pL.y))/(p2.y-p0.y);

    CanvasPoint pR = CanvasPoint(pR_x, pL.y);

    if (hasDepth) {
        int index = floor(pL.y)-floor(p0.y);
        pR.depth = interpolateSingleFloats(p0.depth, p2.depth, floor(p2.y)-floor(p0.y)+1)[index];
    }

    if (hasTexture) {
        int index = floor(pL.y)-floor(p0.y);
        pR.texturePoint = interpolate2DPoints(p0.texturePoint, p2.texturePoint, floor(p2.y)-floor(p0.y)+1)[index];
    }

    if (pL.x > pR.x) std::swap(pL, pR);

    draw2DLine(window, pL, pR, colour_class);

    //TOP HALF TRIANGLE

    // Interpolate p0 and pE
    std::vector<CanvasPoint> p0_pR = Array_2DLine(window, p0, pR);

    // Interpolate p0 and p1
    std::vector<CanvasPoint> p0_pL = Array_2DLine(window, p0, pL);

    int j = 0;
    int jj = 0;
    int iDepths = 0;
    int iTextures = 0;

    std::vector<float> depthsL;
    std::vector<float> depthsR;
    if (hasDepth) {
        depthsL = interpolateSingleFloats(p0.depth, pL.depth, floor(pL.y)-floor(p0.y)+1);
        depthsR = interpolateSingleFloats(p0.depth, pR.depth, floor(pL.y)-floor(p0.y)+1);
    }

    std::vector<TexturePoint> texturesL;
    std::vector<TexturePoint> texturesR;
    if (hasTexture) {
        texturesL = interpolate2DPoints(p0.texturePoint, pL.texturePoint, floor(pL.y)-floor(p0.y)+1);
        texturesR = interpolate2DPoints(p0.texturePoint, pR.texturePoint, floor(pL.y)-floor(p0.y)+1);
    }


    for (float i = floor(p0.y); i < floor(pL.y); i++) {
        while (p0_pL[j].y < i && j < p0_pL.size()) {
            j++;
        }
        while (p0_pR[jj].y < i && jj < p0_pR.size()) {
            jj++;
        }
        if (floor(p0_pL[j].y) == i && floor(p0_pR[jj].y) == i) {
            if (hasDepth && hasTexture) {
                CanvasPoint from = CanvasPoint((p0_pL[j].x), i, depthsL[iDepths]);
                CanvasPoint to = CanvasPoint((p0_pR[jj].x), i, depthsR[iDepths]);
                from.texturePoint = texturesL[iTextures];
                to.texturePoint = texturesR[iTextures];
                draw2DLine(window, from, to, colour_class);
                iTextures++;
                iDepths++;
            } else if (hasDepth) {
                draw2DLine(window, CanvasPoint((p0_pL[j].x), i, depthsL[iDepths]), CanvasPoint((p0_pR[jj].x), i, depthsR[iDepths]), colour_class);
                iDepths++;
            } else if (hasTexture) {
                CanvasPoint from = CanvasPoint((p0_pL[j].x), i);
                CanvasPoint to = CanvasPoint((p0_pR[jj].x), i);
                from.texturePoint = texturesL[iTextures];
                to.texturePoint = texturesR[iTextures];
                draw2DLine(window, from, to, colour_class);
                iTextures++;
            } else {
                draw2DLine(window, CanvasPoint(p0_pL[j].x, i), CanvasPoint(p0_pR[jj].x, i), colour_class);
            }
        }
    }


    // BOTTOM HALF TRIANGLE

    // Interpolate pR and p2
    std::vector<CanvasPoint> pR_p2 = Array_2DLine(window, pR, p2);

    // Interpolate pL and p2
    std::vector<CanvasPoint> pL_p2 = Array_2DLine(window, pL, p2);

    j = 0;
    jj = 0;
    iDepths = 0;
    iTextures = 0;

    if (hasDepth) {
        depthsL = interpolateSingleFloats(pL.depth, p2.depth, floor(p2.y)-floor(pL.y));
        depthsR = interpolateSingleFloats(pR.depth, p2.depth, floor(p2.y)-floor(pL.y));
    }

    if (hasTexture) {
        texturesL = interpolate2DPoints(pL.texturePoint, p2.texturePoint, floor(p2.y)-floor(pL.y)+1);
        texturesR = interpolate2DPoints(pR.texturePoint, p2.texturePoint, floor(p2.y)-floor(pL.y)+1);
    }

    for (float i = floor(pL.y); i < floor(p2.y); i++) {

        while (pL_p2[j].y < i && j < pL_p2.size()) {
            j++;
        }
        while (pR_p2[jj].y < i && jj < pR_p2.size()) {
            jj++;
        }
        if (floor(pL_p2[j].y) == i && floor(pR_p2[jj].y) == i) {
            if (hasDepth && hasTexture) {
                CanvasPoint from = CanvasPoint((pL_p2[j].x), i, depthsL[iDepths]);
                CanvasPoint to = CanvasPoint((pR_p2[jj].x), i, depthsR[iDepths]);
                from.texturePoint = texturesL[iTextures];
                to.texturePoint = texturesR[iTextures];
                draw2DLine(window, from, to, colour_class);
                iTextures++;
                iDepths++;
            } else if (hasDepth) {
                draw2DLine(window, CanvasPoint((pL_p2[j].x), i, depthsL[iDepths]), CanvasPoint((pR_p2[jj].x), i, depthsR[iDepths]), colour_class);
                iDepths++;
            } else if (hasTexture) {
                CanvasPoint from = CanvasPoint((pL_p2[j].x), i);
                CanvasPoint to = CanvasPoint((pR_p2[jj].x), i);
                from.texturePoint = texturesL[iTextures];
                to.texturePoint = texturesR[iTextures];
                draw2DLine(window, from, to, colour_class);
                iTextures++;
            } else {
                draw2DLine(window, CanvasPoint(pL_p2[j].x, i), CanvasPoint(pR_p2[jj].x, i), colour_class);
            }
        }
    }
}


void loadMapTexture(DrawingWindow &window, std::string textureName) {

    TextureMap texture = TextureMap(textureName);
    textureArray.resize(texture.width, std::vector<uint32_t>(texture.height));

    for (int i=0; i<texture.width; i++) {
        for (int j=0; j<texture.height; j++) {
            textureArray[i][j] = texture.pixels[i+j*texture.width];
        }
    }
}


RayTriangleIntersection getClosestValidIntersection(glm::vec3 rayStartCoord, glm::vec3 rayDirection, ModelTriangle shootingTriangle, bool isEmptyTriangle) {

    std::vector<glm::vec3> possibleSolutions;
    std::vector<ModelTriangle> intersectedTriangles;
    std::vector<int> indexes;

    int i = 0;
    for (ModelTriangle triangle : faces) {
        if(!isEmptyTriangle) {
            if (triangle.vertices[0] == shootingTriangle.vertices[0] && triangle.vertices[1] == shootingTriangle.vertices[1] && triangle.vertices[2] == shootingTriangle.vertices[2]) {
                i++;
                continue;
            }
        }
        triangle.vertices[0] = rotationY * rotationX * triangle.vertices[0];
        triangle.vertices[1] = rotationY * rotationX * triangle.vertices[1];
        triangle.vertices[2] = rotationY * rotationX * triangle.vertices[2];
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = rayStartCoord - triangle.vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        float t = possibleSolution.x;
        float u = possibleSolution.y;
        float v = possibleSolution.z;

        // Check if the intersection is within the triangle
        if (u >= 0.0 && (u <= 1.0) && v >= 0.0 && (v <= 1.0) && (u + v) <= 1.0) {
            // Check if the intersection is in the direction of the ray and not behind the camera
            if (t > 0) {
                possibleSolutions.push_back(possibleSolution);
                indexes.push_back(i);
                intersectedTriangles.push_back(faces[i]);
            }
        }
        i++;
    }

    i = 0;

    if (!possibleSolutions.empty()) {
        float minDistance = possibleSolutions[0].x;
        glm::vec3 closestPoint;
        int index = 0;
        for (glm::vec3 point : possibleSolutions) {
            float distance = point.x;
            if (distance < minDistance) {
                minDistance = distance;
                closestPoint = point;
                index = i;
            }
            i++;
        }

        ModelTriangle triangle = intersectedTriangles[index];
        triangle.vertices[0] = rotationY * rotationX * triangle.vertices[0];
        triangle.vertices[1] = rotationY * rotationX * triangle.vertices[1];
        triangle.vertices[2] = rotationY * rotationX * triangle.vertices[2];
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];

        glm::vec3 posSolToCoord = rayStartCoord + rayDirection * minDistance;

        RayTriangleIntersection intersection = RayTriangleIntersection(posSolToCoord, minDistance, intersectedTriangles[index], indexes[index]);
        return intersection;
    }
    return RayTriangleIntersection(glm::vec3(0, 0, 0), 0, ModelTriangle(), -1);
}


CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPositionVar, glm::vec3 vertexPosition, float focalLength) {
    vertexPosition = vertexPosition * rotationX * rotationY;
    glm::vec3 cameraToVertex = vertexPosition - cameraPositionVar;
    cameraToVertex = cameraToVertex * cameraOrientation;

    float ui = round((-(focalLength * cameraToVertex.x) / cameraToVertex.z)*IMAGEPLANE+WIDTH/2);
    float vi = round(((focalLength * cameraToVertex.y) / cameraToVertex.z)*IMAGEPLANE+HEIGHT/2);
    float depth = -1/cameraToVertex.z;

    return CanvasPoint(ui, vi, depth);
}


// Load cornell-box.mtl and populate colourPaletteMap global variable
void loadMtlFile(DrawingWindow &window) {

    std::ifstream file("cornell-box.mtl");
    std::string str;
    while (std::getline(file, str)) {
        std::vector<std::string> strings = split(str, ' ');
        if (strings[0]=="newmtl") {
            std::string colourName = strings[1];
            std::getline(file, str);
            std::vector<std::string> colourStrings = split(str, ' ');
            int r = std::stof(colourStrings[1])*255;
            int g = std::stof(colourStrings[2])*255;
            int b = std::stof(colourStrings[3])*255;
            colourPaletteMap[colourName] = Colour(colourName, r, g, b);
            if (colourName == "Cobbles") {
                std::getline(file, str);
                std::vector<std::string> textureName = split(str, ' ');
                loadMapTexture(window, textureName[1]);
            }
        }
    }
}


// Load cornell-box.obj and read the vertices and faces
void loadObjFile(DrawingWindow &window, std::string fileName) {

    std::string str;
    std::vector<glm::vec3> vertices;
    std::vector<TexturePoint> texturePoints;
    Colour currentColour;

    if (isSphere) {
        currentColour = colourPaletteMap["Red"];
    }
    std::ifstream file(fileName);
    while (std::getline(file, str)) {

        std::vector<std::string> strings = split(str, ' ');

        if (strings[0]=="usemtl") {
            currentColour = colourPaletteMap[strings[1]];
        }
        if (strings[0]=="v") {
            vertices.push_back(glm::vec3(std::stof(strings[1])*0.35, std::stof(strings[2])*0.35, std::stof(strings[3])*0.35));
        }
        if (strings[0]=="vt") {
            texturePoints.push_back(TexturePoint(std::stof(strings[1])*480, std::stof(strings[2])*395));
            std::cout << std::stof(strings[1]) << " " << std::stof(strings[2]) << std::endl;
            std::cout << std::stof(strings[1])*480 << " " << std::stof(strings[2])*395 << std::endl;
        }
        if (strings[0]=="f") {
            ModelTriangle triangle = ModelTriangle();
            if (loadSphere) {
                triangle = ModelTriangle(vertices[std::stoi(split(strings[1], '/')[0])-1]+glm::vec3(0.3, -1.15, 0), vertices[std::stoi(split(strings[2], '/')[0])-1]+glm::vec3(0.3, -1.15, 0), vertices[std::stoi(split(strings[3], '/')[0])-1]+glm::vec3(0.3, -1.15, 0), currentColour);
                triangle.normal = glm::normalize(glm::cross(triangle.vertices[0]-triangle.vertices[2], triangle.vertices[1]-triangle.vertices[2]));
                for (glm::vec3 vertex : triangle.vertices) {
                    std::string vertexAsString = std::to_string(vertex.x) + " " + std::to_string(vertex.y) + " " + std::to_string(vertex.z);
                    if (vertexNormalsMap.find(vertexAsString) == vertexNormalsMap.end()) {
                        vertexNormalsMap[vertexAsString] = std::make_pair(triangle.normal, 1);
                    } else {
                        vertexNormalsMap[vertexAsString].first += triangle.normal;
                        vertexNormalsMap[vertexAsString].second++;
                    }
                }
            } else {
                if (currentColour.name == "Red") {
                    glm::vec3 liftSquareByVec = glm::vec3(0, 0.1, 0);
                    triangle = ModelTriangle(vertices[std::stoi(split(strings[1], '/')[0])-1]+liftSquareByVec, vertices[std::stoi(split(strings[2], '/')[0])-1]+liftSquareByVec, vertices[std::stoi(split(strings[3], '/')[0])-1]+liftSquareByVec, currentColour);
                } else {
                    triangle = ModelTriangle(vertices[std::stoi(split(strings[1], '/')[0])-1], vertices[std::stoi(split(strings[2], '/')[0])-1], vertices[std::stoi(split(strings[3], '/')[0])-1], currentColour);
                }
                if (split(strings[1], '/')[1] != "") {
                    triangle.texturePoints[0] = texturePoints[std::stoi(split(strings[1], '/')[1])-1];
                    triangle.texturePoints[1] = texturePoints[std::stoi(split(strings[2], '/')[1])-1];
                    triangle.texturePoints[2] = texturePoints[std::stoi(split(strings[3], '/')[1])-1];
                }
                triangle.normal = glm::normalize(glm::cross(triangle.vertices[0]-triangle.vertices[2], triangle.vertices[1]-triangle.vertices[2]));

            }
            faces.push_back(triangle);
        }
    }

    // Get an iterator pointing to the first element in the map
    std::map<std::string, std::pair<glm::vec3, int>>::iterator it = vertexNormalsMap.begin();

    // Iterate through the map, and normalize the vertices
    while (it != vertexNormalsMap.end())
    {
        float x = vertexNormalsMap[it->first].first[0]/vertexNormalsMap[it->first].second;
        float y = vertexNormalsMap[it->first].first[1]/vertexNormalsMap[it->first].second;
        float z = vertexNormalsMap[it->first].first[2]/vertexNormalsMap[it->first].second;
        vertexNormalsMap[it->first].first = glm::normalize(glm::vec3(x, y, z));
        ++it;
    }

}


void drawRasterisedScene(DrawingWindow &window) {

    memset(depthsArray, 0, sizeof depthsArray);

    for (ModelTriangle triangle : faces) {
        CanvasPoint p0 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[0], 2);
        CanvasPoint p1 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[1], 2);
        CanvasPoint p2 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[2], 2);

        if (triangle.texturePoints[0].x!=0) {
            p0.texturePoint = triangle.texturePoints[0];
            p1.texturePoint = triangle.texturePoints[1];
            p2.texturePoint = triangle.texturePoints[2];
        }

        if (renderMode == 1) {
            drawFilledTriangle(window, CanvasTriangle(p0, p1, p2), triangle.colour);
        } else if (renderMode==0) {
            drawTriangle(window, CanvasTriangle(p0, p1, p2), triangle.colour);
        }

    }
}


void lookAt(glm::vec3 target) {
    glm::vec3 forward = glm::normalize(cameraPosition - target);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
    glm::vec3 up = glm::cross(forward, right);

    cameraOrientation = glm::mat3(right, up, forward);
}


glm::vec3 pixelToDirectionFromCamera(int x, int y, int width, int height) {

    // Convert from pixel coordinates to normalized device coordinates (range [-1, 1])
    float nx = (0.5f * (x-WIDTH/2)) / IMAGEPLANE;
    float ny = -(0.5f * (y-HEIGHT/2)) / IMAGEPLANE;
    float nz = -1.0f; // Assuming the camera looks towards -Z direction

    // Convert normalized device coordinates to world coordinates
    glm::vec3 rayDirFromCam = glm::normalize(glm::vec3(nx, ny, nz));
    //glm::vec3 rayDirFromCam = glm::vec3(nx, ny, nz);
    return rayDirFromCam;
}


void drawHardShadows(int x, int y, glm::vec3 intersectionPoint, ModelTriangle triangle, glm::vec3 lightSourcePositionVar, Colour colour, DrawingWindow &window) {

    glm::vec3 toLight = lightSourcePositionVar - intersectionPoint;
    float distanceToLight = glm::length(toLight);
    toLight = glm::normalize(toLight);

    RayTriangleIntersection intersectionShadow = getClosestValidIntersection(intersectionPoint, toLight, triangle, false);
    // Check if the intersection point is in shadow
    if (intersectionShadow.distanceFromCamera < distanceToLight && intersectionShadow.triangleIndex != -1) {
        uint32_t  pixelColor = (255 << 24) + (int(colour.red*0.2) << 16) + (int(colour.green*0.2) << 8) + int(colour.blue*0.2);
        window.setPixelColour(x, y, pixelColor);
    }
}


void drawPixelHardShadows(int x, int y, glm::vec3 intersectionPoint, ModelTriangle triangle, glm::vec3 lightSourcePositionVar, Colour colour, DrawingWindow &window) {

    glm::vec3 toLight = lightSourcePositionVar - intersectionPoint;
    float distanceToLight = glm::length(toLight);
    toLight = glm::normalize(toLight);

    RayTriangleIntersection intersectionShadow = getClosestValidIntersection(intersectionPoint, toLight, triangle, false);
    // Check if the intersection point is in shadow
    if (intersectionShadow.distanceFromCamera < distanceToLight && intersectionShadow.triangleIndex != -1) {
        uint32_t  pixelColor = (255 << 24) + (int(colour.red*0.2) << 16) + (int(colour.green*0.2) << 8) + int(colour.blue*0.2);
        window.setPixelColour(x, y, pixelColor);
    } else {
        uint32_t  pixelColor = (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
        window.setPixelColour(x, y, pixelColor);
    }
}


void drawSoftShadows(int x, int y, glm::vec3 intersectionPoint, ModelTriangle triangle, glm::vec3 lightSourcePositionVar, Colour colour, DrawingWindow &window) {

    float lightWidth = 0.455;
    int numberOfRays = 5;

    glm::vec3 lightSourcePositionStart = lightSourcePositionVar - glm::vec3(lightWidth/2, 0, lightWidth/2);

    float shadowIntensity = numberOfRays*numberOfRays;

    for (int i=0; i<numberOfRays; i++) {
        for (int j=0; j<numberOfRays; j++) {
            glm::vec3 toLight = lightSourcePositionStart - intersectionPoint;
            float distanceToLight = glm::length(toLight);
            toLight = glm::normalize(toLight);

            RayTriangleIntersection intersectionShadow = getClosestValidIntersection(intersectionPoint, toLight, triangle, false);
            // Check if the intersection point is in shadow
            if (intersectionShadow.distanceFromCamera < distanceToLight && intersectionShadow.triangleIndex != -1) {
                shadowIntensity--;
            }

            lightSourcePositionStart += glm::vec3(lightWidth/numberOfRays, 0, 0);
        }
        lightSourcePositionStart += glm::vec3(-lightWidth, 0, lightWidth/numberOfRays);
    }

    shadowIntensity = shadowIntensity/(numberOfRays*numberOfRays);

    if (shadowIntensity<0.2) {
        shadowIntensity = 0.2;
    }
    if (shadowIntensity != 1) {
        uint32_t  pixelColor = (255 << 24) + (int(colour.red*shadowIntensity) << 16) + (int(colour.green*shadowIntensity) << 8) + int(colour.blue*shadowIntensity);
        window.setPixelColour(x, y, pixelColor);
    }
}


void drawPixelSoftShadows(int x, int y, glm::vec3 intersectionPoint, ModelTriangle triangle, glm::vec3 lightSourcePositionVar, Colour colour, DrawingWindow &window) {

    float lightWidth = 0.455;
    int numberOfRays = 10;

    glm::vec3 lightSourcePositionStart = lightSourcePositionVar - glm::vec3(lightWidth/2, 0, lightWidth/2);

    float shadowIntensity = numberOfRays*numberOfRays;

    for (int i=0; i<numberOfRays; i++) {
        for (int j=0; j<numberOfRays; j++) {
            glm::vec3 toLight = lightSourcePositionStart - intersectionPoint;
            float distanceToLight = glm::length(toLight);
            toLight = glm::normalize(toLight);

            RayTriangleIntersection intersectionShadow = getClosestValidIntersection(intersectionPoint, toLight, triangle, false);
            // Check if the intersection point is in shadow
            if (intersectionShadow.distanceFromCamera < distanceToLight && intersectionShadow.triangleIndex != -1) {
                shadowIntensity--;
            }

            lightSourcePositionStart += glm::vec3(lightWidth/numberOfRays, 0, 0);
        }
        lightSourcePositionStart += glm::vec3(-lightWidth, 0, lightWidth/numberOfRays);
    }

    shadowIntensity = shadowIntensity/(numberOfRays*numberOfRays);

    if (shadowIntensity<0.2) {
        shadowIntensity = 0.2;
    }

    uint32_t  pixelColor = (255 << 24) + (int(colour.red*shadowIntensity) << 16) + (int(colour.green*shadowIntensity) << 8) + int(colour.blue*shadowIntensity);
    window.setPixelColour(x, y, pixelColor);
}

void drawPixelNoShadows(int x, int y, Colour colour, DrawingWindow &window) {
    uint32_t pixelColor = (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
    window.setPixelColour(x, y, pixelColor);
}

float getLightIntensity(glm::vec3 intersectionPoint, glm::vec3 normal) {

    //Proximity lighting
    float distance = glm::length(lightSourcePosition - intersectionPoint);
    float proximity = 5 / (3*M_PI*(distance * distance));

    //Angle of Incidence lighting
    float angleOfIncidence = glm::dot(normal, glm::normalize(lightSourcePosition - intersectionPoint));

    //Specular lighting
    glm::vec3 reflectionVec = glm::normalize(intersectionPoint - lightSourcePosition) - 2.0f*normal*glm::dot(intersectionPoint - lightSourcePosition, normal);
    float specular = pow(glm::dot(glm::normalize(lightSourcePosition - intersectionPoint), glm::normalize(reflectionVec)), 65);

    proximity = glm::clamp(proximity, 0.0f, 1.0f);
    angleOfIncidence = glm::clamp(angleOfIncidence, 0.0f, 1.0f);
    specular = glm::clamp(specular, 0.0f, 1.0f);

    // BEST Intensity of lighting
    float intensityOfLighting = (proximity*3 + angleOfIncidence*2 + specular)/6;

    if (isSphere) {
        // BEST Intensity of lighting for sphere
        intensityOfLighting = (proximity*3 + angleOfIncidence*2 + specular*6)/11;
    }

    //Ambient lighting
    intensityOfLighting = glm::clamp(intensityOfLighting, 0.2f, 1.0f);

    return intensityOfLighting;
}


// Compute barycentric coordinates (u, v, w) for
// point p with respect to triangle (a, b, c)
glm::vec3 getBarycentricCoordinates(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 p) {
    // Source:
    // https://ceng2.ktu.edu.tr/~cakir/files/grafikler/Texture_Mapping.pdf
    // https://www.sciencedirect.com/book/9781558607323/real-time-collision-detection#book-info
    glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return glm::vec3(u, v, w);
}

float getGouraudShadingIntensity(RayTriangleIntersection intersection) {

    std::vector<float> intensityOfLightingOfVertices;
    for (glm::vec3 vertex : intersection.intersectedTriangle.vertices) {
        std::string vertexAsString = std::to_string(vertex.x) + " " + std::to_string(vertex.y) + " " + std::to_string(vertex.z);
        intensityOfLightingOfVertices.push_back(getLightIntensity(vertex, vertexNormalsMap[vertexAsString].first));
    }

    glm::vec3 barycentricCoordinates = getBarycentricCoordinates(intersection.intersectedTriangle.vertices[0], intersection.intersectedTriangle.vertices[1], intersection.intersectedTriangle.vertices[2], intersection.intersectionPoint);
    float intensityOfLighting = barycentricCoordinates.x*intensityOfLightingOfVertices[0] + barycentricCoordinates.y*intensityOfLightingOfVertices[1] + barycentricCoordinates.z*intensityOfLightingOfVertices[2];
    return intensityOfLighting;
}


float getPhongShadingIntensity(RayTriangleIntersection intersection) {

    std::vector<glm::vec3> vertexNormals;
    for (glm::vec3 vertex : intersection.intersectedTriangle.vertices) {
        std::string vertexAsString = std::to_string(vertex.x) + " " + std::to_string(vertex.y) + " " + std::to_string(vertex.z);
        vertexNormals.push_back(vertexNormalsMap[vertexAsString].first);
    }

    glm::vec3 barycentricCoordinates = getBarycentricCoordinates(intersection.intersectedTriangle.vertices[0], intersection.intersectedTriangle.vertices[1], intersection.intersectedTriangle.vertices[2], intersection.intersectionPoint);
    glm::vec3 interpolatedNormal = barycentricCoordinates.x*vertexNormals[0] + barycentricCoordinates.y*vertexNormals[1] + barycentricCoordinates.z*vertexNormals[2];

    float intensityOfLighting = getLightIntensity(intersection.intersectionPoint, interpolatedNormal);
    return intensityOfLighting;
}

float fresnel(glm::vec3 incidentRay, glm::vec3 surfaceNormal, float indexOfRefraction) {
    // Source:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel.html
    float cosIncidenceAngle = glm::clamp(dot(incidentRay, surfaceNormal), -1.0f, 1.0f);
    float etaIncident = 1, etaTransmitted = indexOfRefraction;
    if (cosIncidenceAngle > 0) { std::swap(etaIncident, etaTransmitted); }
    // Compute sinTransmitted using Snell's law
    float sinTransmitted = etaIncident / etaTransmitted * sqrtf(std::max(0.f, 1 - cosIncidenceAngle * cosIncidenceAngle));
    // Total internal reflection
    if (sinTransmitted >= 1) {
        float kr = 1;
        return kr;
    }
    else {
        float cosTransmitted = sqrtf(std::max(0.f, 1 - sinTransmitted * sinTransmitted));
        cosIncidenceAngle = fabsf(cosIncidenceAngle);
        float reflectancePerpendicular = ((etaTransmitted * cosIncidenceAngle) - (etaIncident * cosTransmitted)) / ((etaTransmitted * cosIncidenceAngle) + (etaIncident * cosTransmitted));
        float reflectanceParallel = ((etaIncident * cosIncidenceAngle) - (etaTransmitted * cosTransmitted)) / ((etaIncident * cosIncidenceAngle) + (etaTransmitted * cosTransmitted));
        float kr = (reflectancePerpendicular * reflectancePerpendicular + reflectanceParallel * reflectanceParallel) / 2;
        return kr;
    }
}

glm::vec3 refract(glm::vec3 rayDirection, glm::vec3 surfaceNormal, float refractiveIndex) {
    // Source:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel.html

    float cosi = glm::clamp(dot(rayDirection, surfaceNormal), -1.0f, 1.0f);
    float etai = 1;
    float etat = refractiveIndex;
    glm::vec3 n = surfaceNormal;

    if (cosi < 0) {
        cosi = -cosi;
    } else {
        std::swap(etai, etat); n= -surfaceNormal;
    }

    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);

    if (k < 0) {
        // total internal reflection. There is no refraction in this case
        return glm::vec3(0, 0, 0);
    } else {
        return eta * rayDirection + (eta * cosi - sqrtf(k)) * n;
    }
}

void drawRayTracedScene(DrawingWindow &window) {

    window.clearPixels();
    int maxNoOfReflectGlobal = 0;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Convert pixel coordinates to a ray direction
            glm::vec3 rayDir = pixelToDirectionFromCamera(x, y, WIDTH, HEIGHT);
            rayDir = cameraOrientation * rayDir; // Apply camera orientation

            // Find the closest intersection
            RayTriangleIntersection intersection = getClosestValidIntersection(cameraPosition, rayDir, ModelTriangle(), true);

            if (intersection.triangleIndex!=-1) {

                Colour colour = intersection.intersectedTriangle.colour;

                if (colour.name == "Red" && isSphere) {
                    float intensityOfLighting;

                    if (changeToPhong) {
                        intensityOfLighting = getPhongShadingIntensity(intersection);
                    } else {
                        intensityOfLighting = getGouraudShadingIntensity(intersection);
                    }


                    colour.red = colour.red*intensityOfLighting;
                    colour.green = colour.green*intensityOfLighting;
                    colour.blue = colour.blue*intensityOfLighting;

                    drawPixelNoShadows(x, y, colour, window);
                }
                // Set colour name to colour of object that will be the mirror
                else if (colour.name == "Yelloww") {
                    glm::vec3 vectorOfReflection = rayDir - 2.0f*intersection.intersectedTriangle.normal*glm::dot(rayDir, intersection.intersectedTriangle.normal);
                    RayTriangleIntersection intersectionFromMirror = getClosestValidIntersection(intersection.intersectionPoint, vectorOfReflection, intersection.intersectedTriangle, false);
                    float intensityOfLighting = getLightIntensity(intersectionFromMirror.intersectionPoint, intersectionFromMirror.intersectedTriangle.normal);

                    colour.red = intersectionFromMirror.intersectedTriangle.colour.red*intensityOfLighting;
                    colour.green = intersectionFromMirror.intersectedTriangle.colour.green*intensityOfLighting;
                    colour.blue = intersectionFromMirror.intersectedTriangle.colour.blue*intensityOfLighting;

                    drawPixelNoShadows(x, y, colour, window);
                    //drawPixelHardShadows(x, y, intersectionFromMirror.intersectionPoint, intersectionFromMirror.intersectedTriangle, lightSourcePosition, colour, window);
                    //drawPixelSoftShadows(x, y, intersectionFromMirror.intersectionPoint, intersectionFromMirror.intersectedTriangle, lightSourcePosition, colour, window);
                }
                //Set colour name to colour of object that will be the glass
                else if (colour.name == "Redd" && !isSphere) {

                    glm::vec3 refractedRayDir = glm::normalize(refract(rayDir, intersection.intersectedTriangle.normal, refractiveIndexGlobal));

                    if (refractedRayDir == glm::vec3(0, 0, 0)) {
                        // total reflection, there is no refraction
                        glm::vec3 vectorOfReflection = rayDir - 2.0f*intersection.intersectedTriangle.normal*glm::dot(rayDir, intersection.intersectedTriangle.normal);
                        RayTriangleIntersection intersectionFromMirror = getClosestValidIntersection(intersection.intersectionPoint, vectorOfReflection, intersection.intersectedTriangle, false);
                        float intensityOfLighting = getLightIntensity(intersectionFromMirror.intersectionPoint, intersectionFromMirror.intersectedTriangle.normal);

                        colour.red = intersectionFromMirror.intersectedTriangle.colour.red*intensityOfLighting;
                        colour.green = intersectionFromMirror.intersectedTriangle.colour.green*intensityOfLighting;
                        colour.blue = intersectionFromMirror.intersectedTriangle.colour.blue*intensityOfLighting;

                        drawPixelNoShadows(x, y, colour, window);
                    } else {
                        int maxNumberOfReflections;
                        bool inside = true;

                        int i=0;

                        Colour refractionColour = Colour(0, 0, 0);
                        Colour reflectionColour = Colour(0, 0, 0);
                        float kr;

                        while (inside) {
                            kr = fresnel(refractedRayDir, intersection.intersectedTriangle.normal, refractiveIndexGlobal);

                            RayTriangleIntersection intersectionFromPointOfRefraction = getClosestValidIntersection(intersection.intersectionPoint, refractedRayDir, intersection.intersectedTriangle, false);
                            glm::vec3 refractedRayDirFromPointOfRefraction = glm::normalize(refract(refractedRayDir, intersectionFromPointOfRefraction.intersectedTriangle.normal, refractiveIndexGlobal));

                            float dotProduct = glm::dot(glm::normalize(refractedRayDir), intersectionFromPointOfRefraction.intersectedTriangle.normal);
                            float angleOfIncidence = glm::acos(dotProduct) * 180.0f / M_PI;
                            float criticalAngle = glm::asin(1.0f / refractiveIndexGlobal) * 180.0f / M_PI;

                            if (angleOfIncidence > criticalAngle) {
                                // total reflection, there is no refraction, but since it is inside the cube, we get the colour of refraction, so we can scale it later

                                glm::vec3 vectorOfReflectionInsideGlass = refractedRayDir - 2.0f*(-intersectionFromPointOfRefraction.intersectedTriangle.normal)*glm::dot(refractedRayDir, (-intersectionFromPointOfRefraction.intersectedTriangle.normal));

                                intersection = intersectionFromPointOfRefraction;

                                RayTriangleIntersection intersectionOutsideGlass = getClosestValidIntersection(intersection.intersectionPoint, refractedRayDir, intersection.intersectedTriangle, false);
                                float intensityOfLighting = getLightIntensity(intersectionOutsideGlass.intersectionPoint, intersectionOutsideGlass.intersectedTriangle.normal);

                                reflectionColour.red += intersectionOutsideGlass.intersectedTriangle.colour.red*intensityOfLighting*kr;
                                reflectionColour.green += intersectionOutsideGlass.intersectedTriangle.colour.green*intensityOfLighting*kr;
                                reflectionColour.blue += intersectionOutsideGlass.intersectedTriangle.colour.blue*intensityOfLighting*kr;

                                refractedRayDir = vectorOfReflectionInsideGlass;

                            } else {

                                intersection = intersectionFromPointOfRefraction;

                                refractedRayDir = glm::normalize(refract(refractedRayDir, intersectionFromPointOfRefraction.intersectedTriangle.normal, refractiveIndexGlobal));

                                RayTriangleIntersection intersectionOutsideGlass = getClosestValidIntersection(intersection.intersectionPoint, refractedRayDir, intersection.intersectedTriangle, false);
                                float intensityOfLighting = getLightIntensity(intersectionOutsideGlass.intersectionPoint, intersectionOutsideGlass.intersectedTriangle.normal);

                                refractionColour.red = intersectionOutsideGlass.intersectedTriangle.colour.red*intensityOfLighting*(1-kr);
                                refractionColour.green = intersectionOutsideGlass.intersectedTriangle.colour.green*intensityOfLighting*(1-kr);
                                refractionColour.blue = intersectionOutsideGlass.intersectedTriangle.colour.blue*intensityOfLighting*(1-kr);

                                inside = false;
                            }
                            i++;

                            if (i>maxNoOfReflectGlobal) {
                                maxNoOfReflectGlobal = i;
                            }

                            maxNumberOfReflections = 16;
                            if (i==maxNumberOfReflections) {
                                inside = false;
                            }
                        }

                        colour.red = reflectionColour.red/(maxNumberOfReflections-1) + refractionColour.red;
                        colour.green = reflectionColour.green/(maxNumberOfReflections-1) + refractionColour.green;
                        colour.blue = reflectionColour.blue/(maxNumberOfReflections-1) + refractionColour.blue;

                        drawPixelNoShadows(x, y, colour, window);

                    }

                }
                else {
                    float intensityOfLighting = getLightIntensity(intersection.intersectionPoint, intersection.intersectedTriangle.normal);

                    colour.red = colour.red*intensityOfLighting;
                    colour.green = colour.green*intensityOfLighting;
                    colour.blue = colour.blue*intensityOfLighting;

                    drawPixelNoShadows(x, y, colour, window);
                    //drawPixelHardShadows(x, y, intersection.intersectionPoint, intersection.intersectedTriangle, lightSourcePosition, colour, window);
                    //drawPixelSoftShadows(x, y, intersection.intersectionPoint, intersection.intersectedTriangle, lightSourcePosition, colour, window);
                }

            }
        }
    }
    std::cout << "Scene Drawn" << std::endl;
}


void draw(DrawingWindow &window) {
    window.clearPixels();
    if (renderMode == 0) {
        drawRasterisedScene(window);
    } else if (renderMode == 1) {
        drawRasterisedScene(window);
    } else {
        drawRayTracedScene(window);
    }
}


float cameraSpeed = M_PI/24; // Adjust the speed of the orbit as needed
float cameraAngle = 0;  // Initial angle

void orbit(DrawingWindow &window, SDL_Event &event) {

    if (cameraAngle >= 2*M_PI){

        // Reset to 0 when it gets to 2pi
        cameraAngle = 0;
        paused = true;
    } else {
        cameraAngle = cameraAngle + cameraSpeed;
        positionY = glm::mat3(
                cos(cameraAngle), 0.0, sin(cameraAngle),
                0.0, 1.0, 0.0,
                -sin(cameraAngle), 0.0, cos(cameraAngle)
        );
        cameraPosition = initialCameraPosition * positionY;
    }

    lookAt(glm::vec3(0, 0, 0));
    draw(window);

}


void handleEvent(SDL_Event event, DrawingWindow &window) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT){
            std::cout << "LEFT" << std::endl;

            //float angle = -M_PI/6;
            //setRotationAngle('y', angle);

            float angle = -M_PI/12;
            setOrientationAngle('y', angle);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_RIGHT){
            std::cout << "RIGHT" << std::endl;

            //float angle = M_PI/6;
            //setRotationAngle('y', angle);

            float angle = M_PI/12;
            setOrientationAngle('y', angle);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_UP){
            std::cout << "UP" << std::endl;

            //float angle = -M_PI/6;
            //setRotationAngle('x', angle);

            float angle = -M_PI/12;
            setOrientationAngle('x', angle);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_DOWN){
            std::cout << "DOWN" << std::endl;

            //float angle = M_PI/6;
            //setRotationAngle('x', angle);

            float angle = M_PI/12;
            setOrientationAngle('x', angle);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_t) {
            std::cout << "t" << std::endl;

            //float angle = -M_PI/12;
            //setOrientationAngle('x', angle);
            lightSourcePosition = lightSourcePosition + glm::vec3(0, 0.25, 0);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_g) {
            std::cout << "g" << std::endl;

            //float angle = M_PI/12;
            //setOrientationAngle('x', angle);
            lightSourcePosition = lightSourcePosition + glm::vec3(0, -0.25, 0);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_f) {
            std::cout << "f" << std::endl;

            //float angle = -M_PI/12;
            //setOrientationAngle('y', angle);
            lightSourcePosition = lightSourcePosition + glm::vec3(-0.25, 0, 0);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_h) {
            std::cout << "h" << std::endl;

            //float angle = M_PI/12;
            //setOrientationAngle('y', angle);
            lightSourcePosition = lightSourcePosition + glm::vec3(0.25, 0, 0);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_r) {
            std::cout << "r" << std::endl;

            //float angle = -M_PI/12;
            //setOrientationAngle('y', angle);
            lightSourcePosition = lightSourcePosition + glm::vec3(0, 0, -0.25);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_y) {
            std::cout << "y" << std::endl;

            //float angle = M_PI/12;
            //setOrientationAngle('y', angle);
            lightSourcePosition = lightSourcePosition + glm::vec3(0, 0, 0.25);
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_w) {
            std::cout << "w" << std::endl;

            cameraPosition[1]-=1;
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_s) {
            std::cout << "s" << std::endl;

            cameraPosition[1]+=1;
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_a) {
            std::cout << "a" << std::endl;

            cameraPosition[0]+=1;
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_d) {
            std::cout << "d" << std::endl;

            cameraPosition[0]-=1;
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_q) {
            std::cout << "q" << std::endl;

            cameraPosition[2] += 1;
            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_e) {
            std::cout << "e" << std::endl;

            cameraPosition[2]-=1;

            draw(window);
        }
        else if (event.key.keysym.sym == SDLK_u) {
            std::cout << "u" << std::endl;

            CanvasPoint p1 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p2 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p3 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            Colour colour = Colour(rand()%256, rand()%256, rand()%256);
            drawTriangle(window, CanvasTriangle(p1, p2, p3), colour);

        }
        else if (event.key.keysym.sym == SDLK_j) {
            std::cout << "j" << std::endl;

            CanvasPoint p1 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p2 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p3 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            Colour colour = Colour(rand()%256, rand()%256, rand()%256);
            drawFilledTriangle(window, CanvasTriangle(p1, p2, p3), colour);

        } else if (event.key.keysym.sym == SDLK_l) {
            std::cout << "l" << std::endl;
            renderMode++;
            if (renderMode >= 3) {
                renderMode = 0;
            }

            draw(window);

        } else if (event.key.keysym.sym == SDLK_p) {
            std::cout << "p" << std::endl;

            if (paused) {
                paused = false;
            } else {
                paused = true;
            }
        }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        window.savePPM("output.ppm");
        window.saveBMP("output.bmp");
    }
}

void drawAnimate(DrawingWindow &window) {
    window.clearPixels();


    cameraPosition[0]-=0.05;
    cameraPosition[1]+=0.05;

    cameraPosition[2]-=0.25;

    draw(window);
    lookAtGlobal = lookAtGlobal + glm::vec3(-0.05, 0.025, 0.25);
    lookAt(lookAtGlobal);
}

int main(int argc, char *argv[]) {
    DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
    SDL_Event event;
    loadMtlFile(window);
    if (isSphere) {
        cameraPosition = glm::vec3(0, 0, 4);
        initialCameraPosition = cameraPosition;

        // Sphere in a cornell box
        loadObjFile(window, "cornell-box-no-red-box.obj");

        loadSphere = true;
        loadObjFile(window, "sphere.obj");
    } else {
        if (loadClosedCornellBox) {
            loadObjFile(window, "cornell-box-closed.obj");
        } else {
            loadObjFile(window, "cornell-box.obj");
        }
    }

    lookAt(lookAtGlobal);
    draw(window);

    int i = 0;
    while (true) {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(event)) handleEvent(event, window);

        if (!paused) {
            orbit(window, event);

//            drawAnimate(window);
//            i++;
//            window.renderFrame();
//            std::string ppmName = "frames/a" + std::to_string(118-i) + ".ppm";
//            std::string bmpName = "frames/a" + std::to_string(118-i) + ".bmp";
//            window.savePPM(ppmName);
//            window.saveBMP(bmpName);
//            i++;
//            std::string ppmNamee = "frames/a" + std::to_string(118-i) + ".ppm";
//            std::string bmpNamee = "frames/a" + std::to_string(118-i) + ".bmp";
//            window.savePPM(ppmNamee);
//            window.saveBMP(bmpNamee);
//            orbitCount++;
//            if (i==16) {
//                paused = true;
//            }
        }

        window.renderFrame();
    }
}
