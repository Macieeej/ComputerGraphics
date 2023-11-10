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

#define WIDTH 320*4
#define HEIGHT 240*4
#define IMAGEPLANE 240

bool paused = true;

std::map<std::string, Colour> colourPaletteMap;
std::vector<ModelTriangle> faces;
float depthsArray[WIDTH+1][HEIGHT+1] = {0};

glm::vec3 cameraPosition = glm::vec3(0, 0, 4);
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

void drawRedNoise(DrawingWindow &window) {
    window.clearPixels();
    for (size_t y = 0; y < window.height; y++) {
        for (size_t x = 0; x < window.width; x++) {
            float red = rand() % 256;
            float green = 0.0;
            float blue = 0.0;
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}

void drawGreyScale(DrawingWindow &window) {
    window.clearPixels();
    for (size_t y = 0; y < window.height; y++) {

        std::vector<float> result;
        result = interpolateSingleFloats(255, 0, window.width);

        for (size_t x = 0; x < window.width; x++) {
            float red = result[x];
            float green = result[x];
            float blue = result[x];
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}

void draw2DColour(DrawingWindow &window) {
    window.clearPixels();

    glm::vec3 topLeft(255, 0, 0);        // red
    glm::vec3 topRight(0, 0, 255);       // blue
    glm::vec3 bottomRight(0, 255, 0);    // green
    glm::vec3 bottomLeft(255, 255, 0);   // yellow

    std::vector<glm::vec3> result_r_y;
    std::vector<glm::vec3> result_b_g;

    result_r_y = interpolateThreeElementValues(glm::vec3(255, 0, 0), glm::vec3(255, 255, 0), window.height);
    result_b_g = interpolateThreeElementValues(glm::vec3(0, 0, 255), glm::vec3(0, 255, 0), window.height);

    for (size_t y = 0; y < window.height; y++) {

        std::vector<glm::vec3> result_left_right;
        result_left_right = interpolateThreeElementValues(result_r_y[y], result_b_g[y], window.width);

        for (size_t x = 0; x < window.width; x++) {
            float red = result_left_right[x][0];
            float green = result_left_right[x][1];
            float blue = result_left_right[x][2];
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}

void draw2DLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour_class) {

    bool hasDepth = false;
    if (from.depth) {
        hasDepth = true;
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

    if (numberOfSteps!=0) {


        for (float i = 0.0; i <= numberOfSteps; i++) {

            float x = fromX + (xStepSize * i);
            float y = fromY + (yStepSize * i);

            float red = colour_class.red;
            float green = colour_class.green;
            float blue = colour_class.blue;
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);


            if (hasDepth && x>=0 && y>=0 && x<WIDTH && y<HEIGHT) {
                float depth = depths[i];

                if (depth >= depthsArray[(int) floor(x)][(int) floor(y)] ) {
                    depthsArray[(int) floor(x)][(int) floor(y)] = depth;
                    window.setPixelColour(floor(x), floor(y), colour);
                }
            } else {
                //window.setPixelColour(floor(x), floor(y), colour);
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

    if (triangle[0].depth) {
        hasDepth = true;
    }

    //std::cout << "Triangle of colour: " << colour_class.name << std::endl;

    //Colour white = Colour(255, 255, 255);
    //drawTriangle(window, triangle, white);

    CanvasPoint p0 = CanvasPoint(triangle[0].x, triangle[0].y);
    CanvasPoint pL = CanvasPoint(triangle[1].x, triangle[1].y);
    CanvasPoint p2 = CanvasPoint(triangle[2].x, triangle[2].y);

    if (hasDepth) {
        p0.depth = triangle[0].depth;
        pL.depth = triangle[1].depth;
        p2.depth = triangle[2].depth;
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

    if (pL.x > pR.x) std::swap(pL, pR);

    draw2DLine(window, pL, pR, colour_class);

    //TOP HALF TRIANGLE

    // Interpolate p0 and pE
    std::vector<CanvasPoint> p0_pR = Array_2DLine(window, p0, pR);

    // Interpolate p0 and p1
    std::vector<CanvasPoint> p0_pL = Array_2DLine(window, p0, pL);

    int j = 0;
    int jj = 0;
    int idepths = 0;

    std::vector<float> depthsL;
    std::vector<float> depthsR;
    if (hasDepth) {
        depthsL = interpolateSingleFloats(p0.depth, pL.depth, floor(pL.y)-floor(p0.y)+1);
        depthsR = interpolateSingleFloats(p0.depth, pR.depth, floor(pL.y)-floor(p0.y)+1);
    }

    for (float i = floor(p0.y); i < floor(pL.y); i++) {
        while (p0_pL[j].y < i && j < p0_pL.size()) {
            j++;
        }
        while (p0_pR[jj].y < i && jj < p0_pR.size()) {
            jj++;
        }
        if (floor(p0_pL[j].y) == i && floor(p0_pR[jj].y) == i) {
            //draw2DLine(window, CanvasPoint(floor(p0_pL[j].x), floor(p0_pL[j].y)), CanvasPoint(floor(p0_pR[jj].x), floor(p0_pR[jj].y)), colour_class);
            if (hasDepth) {
                draw2DLine(window, CanvasPoint((p0_pL[j].x), i, depthsL[idepths]), CanvasPoint((p0_pR[jj].x), i, depthsR[idepths]), colour_class);
                idepths++;
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
    idepths = 0;

    if (hasDepth) {
        depthsL = interpolateSingleFloats(pL.depth, p2.depth, floor(p2.y)-floor(pL.y));
        depthsR = interpolateSingleFloats(pR.depth, p2.depth, floor(p2.y)-floor(pL.y));
    }

    for (float i = floor(pL.y); i < floor(p2.y); i++) {

        while (pL_p2[j].y < i && j < pL_p2.size()) {
            j++;
        }
        while (pR_p2[jj].y < i && jj < pR_p2.size()) {
            jj++;
        }
        if (floor(pL_p2[j].y) == i && floor(pR_p2[jj].y) == i) {
            //draw2DLine(window, CanvasPoint(pL_p2[j].x, i), CanvasPoint(pR_p2[jj].x, i), colour_class);
            if (hasDepth) {
                draw2DLine(window, CanvasPoint((pL_p2[j].x), i, depthsL[idepths]), CanvasPoint((pR_p2[jj].x), i, depthsR[idepths]), colour_class);
                idepths++;
            } else {
                draw2DLine(window, CanvasPoint(pL_p2[j].x, i), CanvasPoint(pR_p2[jj].x, i), colour_class);
            }

        }

    }
    drawTriangle(window, triangle, colour_class);

}


void loadMapTexture(DrawingWindow &window, CanvasTriangle triangle, CanvasTriangle textureTriangle) {

    TextureMap texture = TextureMap("texture.ppm");

    std::vector<std::vector<TexturePoint>> textureArray;

    //CanvasPoint textureArray[texture.width][texture.height];

    for (int i=0; i<texture.width; i++) {
        std::vector<TexturePoint> row;
        for (int j=0; j<texture.height; j++) {
            //row.push_back(CanvasPoint(i, j));
            //textureArray[i][j] = texture.pixels[i+j*texture.width];
        }
    }

}



RayTriangleIntersection getClosestValidIntersection(glm::vec3 cameraPositionVar, glm::vec3 rayDirection) {

    std::vector<glm::vec3> possibleSolutions;
    std::vector<ModelTriangle> intersectedTriangles;
    std::vector<int> indexes;

    int i = 0;
    for (ModelTriangle triangle : faces) {
        triangle.vertices[0] = triangle.vertices[0] * rotationX * rotationY;
        triangle.vertices[1] = triangle.vertices[1] * rotationX * rotationY;
        triangle.vertices[2] = triangle.vertices[2] * rotationX * rotationY;
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = cameraPositionVar - triangle.vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        float t = possibleSolution.x;
        float u = possibleSolution.y;
        float v = possibleSolution.z;

        // Check if the intersection is within the triangle
        if (u >= 0.0 && (u <= 1.0) && v >= 0.0 && (v <= 1.0) && (u + v) <= 1.0) {
            // Check if the intersection is in the direction of the ray and not behind the camera
            if (t > 0) {
                //glm::vec3 intersectionPoint = rayOrigin + t * rayDirection;
                possibleSolutions.push_back(possibleSolution);
                indexes.push_back(i);
                intersectedTriangles.push_back(triangle);
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
        RayTriangleIntersection intersection = RayTriangleIntersection(closestPoint, minDistance, intersectedTriangles[index], indexes[index]);
        return intersection;
    }

    //throw std::invalid_argument("no intersection found");
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
        }
    }

    // Get an iterator pointing to the first element in the map
    std::map<std::string, Colour>::iterator it = colourPaletteMap.begin();

    // Iterate through the map and print the elements
    /*while (it != colourPaletteMap.end())
    {
        std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
        ++it;
    }*/
}

// Load cornell-box.obj and read the vertices and faces
void loadObjFile(DrawingWindow &window) {


    std::ifstream file("cornell-box.obj");
    std::string str;
    std::vector<glm::vec3> vertices;

    Colour currentColour;

    while (std::getline(file, str)) {

        std::vector<std::string> strings = split(str, ' ');

        if (strings[0]=="usemtl") {
            currentColour = colourPaletteMap[strings[1]];
        }
        if (strings[0]=="v") {
            vertices.push_back(glm::vec3(std::stof(strings[1])*0.35, std::stof(strings[2])*0.35, std::stof(strings[3])*0.35));
        }
        if (strings[0]=="f") {
            ModelTriangle triangle = ModelTriangle(vertices[std::stoi(split(strings[1], '/')[0])-1], vertices[std::stoi(split(strings[2], '/')[0])-1], vertices[std::stoi(split(strings[3], '/')[0])-1], currentColour);
            faces.push_back(triangle);
        }
    }

    /* memset(depthsArray, 0, sizeof depthsArray);

   for (ModelTriangle triangle : faces) {
         CanvasPoint p0 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[0], 2);
         CanvasPoint p1 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[1], 2);
         CanvasPoint p2 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[2], 2);

         drawFilledTriangle(window, CanvasTriangle(p0, p1, p2), triangle.colour);
     }*/


}

void drawRasterisedScene(DrawingWindow &window) {

    memset(depthsArray, 0, sizeof depthsArray);

    for (ModelTriangle triangle : faces) {
        CanvasPoint p0 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[0], 2);
        CanvasPoint p1 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[1], 2);
        CanvasPoint p2 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[2], 2);

        drawFilledTriangle(window, CanvasTriangle(p0, p1, p2), triangle.colour);
    }
}

void lookAt(glm::vec3 target) {
    glm::vec3 forward = glm::normalize(cameraPosition - target);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
    glm::vec3 up = glm::cross(forward, right);

    cameraOrientation = glm::mat3(right, up, forward);
}

glm::vec3 pixelToDirection(int x, int y, int width, int height) {
    // Convert from pixel coordinates to normalized device coordinates (range [-1, 1])
    //std::cout << "x: " << x << " y: " << y << std::endl;
    float nx = (0.5f * (x-WIDTH/2)) / IMAGEPLANE;
    float ny = -(0.5f * (y-HEIGHT/2)) / IMAGEPLANE;
    float nz = -1.0f; // Assuming the camera looks towards -Z direction

    //nx = (((x-WIDTH/2)/IMAGEPLANE)*nz)/-2;
    //nx = ((x-WIDTH/2)/IMAGEPLANE)/2
    // Convert normalized device coordinates to world coordinates
    glm::vec3 rayDir = glm::normalize(glm::vec3(nx, ny, nz));
    //glm::vec3 rayDir = glm::vec3(nx, ny, nz);
    return rayDir;
}

void drawRayTracedScene(DrawingWindow &window) {
    window.clearPixels();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Convert pixel coordinates to a ray direction
            glm::vec3 rayDir = pixelToDirection(x, y, WIDTH, HEIGHT);
            //glm::vec3 rayDir = pixelToDirection(x, y, IMAGEPLANE, IMAGEPLANE);
            rayDir = rayDir * cameraOrientation; // Apply camera orientation

            // Find the closest intersection
            RayTriangleIntersection intersection = getClosestValidIntersection(cameraPosition, rayDir);
            if (intersection.triangleIndex!=-1) {
                // Color the pixel with the color of the intersected triangle
                Colour colour = intersection.intersectedTriangle.colour;
                uint32_t pixelColor = (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
                window.setPixelColour(x, y, pixelColor);
            }
        }
    }
}



//float cameraRadius = 4.0;  // Adjust the radius as needed
float cameraSpeed = M_PI/36; // Adjust the speed of the orbit as needed
float cameraAngle = 0;  // Initial angle

void draw(DrawingWindow &window, SDL_Event &event) {

    //sleep(1);

    //lookAt(glm::vec3(0, 0, 0));
    //std::cout << glm::to_string(cameraOrientation) << std::endl;

    //cameraAngle += cameraSpeed;  // Increase the angle each frame
    if (cameraAngle >= 2*M_PI){
        std::cout << "FULL CIRCLE" << std::endl;
        // Reset to 0 when it gets to 2pi (otherwise it'll just get bigger and bigger
        cameraAngle = 0;
        //cameraPosition = glm::vec3(0, 0, 4);

    } else {
        positionY = glm::mat3(
                cos(cameraAngle), 0.0, sin(cameraAngle),
                0.0, 1.0, 0.0,
                -sin(cameraAngle), 0.0, cos(cameraAngle)
        );
        cameraAngle = cameraAngle + cameraSpeed;

        //float cameraX = cameraRadius * sin(cameraAngle);
        //float cameraZ = cameraRadius * cos(cameraAngle);
        cameraPosition = glm::vec3(0, 0, 4) * positionY;
    }
    lookAt(glm::vec3(0, 0, 0));

    window.clearPixels();
    //drawRasterisedScene(window);
    drawRayTracedScene(window);
}



void handleEvent(SDL_Event event, DrawingWindow &window) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT){
            std::cout << "LEFT" << std::endl;

            float angle = -M_PI/6;
            setRotationAngle('y', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_RIGHT){
            std::cout << "RIGHT" << std::endl;

            float angle = M_PI/6;
            setRotationAngle('y', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_UP){
            std::cout << "UP" << std::endl;

            float angle = -M_PI/6;
            setRotationAngle('x', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_DOWN){
            std::cout << "DOWN" << std::endl;

            float angle = M_PI/6;
            setRotationAngle('x', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_t) {
            std::cout << "t" << std::endl;

            float angle = -M_PI/12;
            setOrientationAngle('x', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_g) {
            std::cout << "g" << std::endl;

            float angle = M_PI/12;
            setOrientationAngle('x', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_f) {
            std::cout << "f" << std::endl;

            float angle = -M_PI/12;
            setOrientationAngle('y', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_h) {
            std::cout << "h" << std::endl;

            float angle = M_PI/12;
            setOrientationAngle('y', angle);
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_w) {
            std::cout << "w" << std::endl;

            cameraPosition[1]-=1;
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_s) {
            std::cout << "s" << std::endl;

            cameraPosition[1]+=1;
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_a) {
            std::cout << "a" << std::endl;

            cameraPosition[0]+=1;
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_d) {
            std::cout << "d" << std::endl;

            cameraPosition[0]-=1;
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_q) {
            cameraPosition[2] += 1;
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
        }
        else if (event.key.keysym.sym == SDLK_e) {
            cameraPosition[2]-=1;
            window.clearPixels();
            //drawRasterisedScene(window);
            drawRayTracedScene(window);
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
            //drawFilledTriangle(window, CanvasTriangle(CanvasPoint(293.923, 129.97), CanvasPoint(348.677, 119.33), CanvasPoint(291.411, 119.33)), colour);


        } else if (event.key.keysym.sym == SDLK_l) {
            std::cout << "l" << std::endl;

            //drawRasterisedScene(window);
            drawRayTracedScene(window);
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

int main(int argc, char *argv[]) {

    /*
    // Test interpolateSingleFloats
    std::vector<float> result;
    result = interpolateSingleFloats(2.2, 8.5, 7);
    for(size_t i=0; i<result.size(); i++) std::cout << result[i] << " ";
    std::cout << std::endl;

    // Test interpolateThreeElementValues
    std::vector<glm::vec3> result3;
    result3 = interpolateThreeElementValues(glm::vec3(0, 0, 255), glm::vec3(0, 255, 0), 4);
    for(size_t i=0; i<result3.size(); i++){

        std::cout << "(" << result3[i][0] << ", " << result3[i][1] << ", " << result3[i][2] << ")";
        std::cout << std::endl;
    }

    */
    DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
    SDL_Event event;
    loadMtlFile(window);
    loadObjFile(window);
    drawRayTracedScene(window);

    while (true) {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(event)) handleEvent(event, window);
        if (!paused) {
            draw(window, event);
        }
        //RayTriangleIntersection tr = getClosestValidIntersection(cameraPosition, glm::vec3(-0.2, 0, -1));
        //std::cout << tr.triangleIndex << std::endl;
        //if (tr.triangleIndex == -1) {
        //    std::cout << "-1 detected" << std::endl;
        //}
        /*
        // Texture mapping, visual verification
        Colour white = Colour(255, 255, 255);
        CanvasTriangle triangle = CanvasTriangle(CanvasPoint(160, 10), CanvasPoint(300, 230), CanvasPoint(10, 150));
        drawTriangle(window, triangle, white);
        //CanvasPoint p0 = CanvasPoint(195, 5);
        //CanvasPoint p1 = CanvasPoint(395, 380);
        //CanvasPoint p2 = CanvasPoint(65, 330);
        CanvasTriangle textureTriangle = CanvasTriangle(CanvasPoint(195, 5), CanvasPoint(395, 380), CanvasPoint(65, 330));
        drawTriangle(window, textureTriangle, white);
        loadMapTexture(window, triangle, textureTriangle);
         */


        // draw2DLine(window, CanvasPoint(0,0), CanvasPoint(WIDTH/2,HEIGHT/2), Colour(255,255,255));
        // draw2DLine(window, CanvasPoint(WIDTH,0), CanvasPoint(WIDTH/2,HEIGHT/2), Colour(255,255,255));
        // draw2DLine(window, CanvasPoint(WIDTH/2,0), CanvasPoint(WIDTH/2,HEIGHT), Colour(255,255,255));
        // draw2DLine(window, CanvasPoint(WIDTH/3,HEIGHT/2), CanvasPoint(WIDTH*2/3,HEIGHT/2), Colour(255,255,255));

        // draw2DColour(window);

        // drawGreyScale(window);

        // drawRedNoise(window);

        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }

}
