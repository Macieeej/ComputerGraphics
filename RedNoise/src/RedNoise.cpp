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

#define WIDTH 320*4
#define HEIGHT 240*4
std::vector<Colour> colourPalette;

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
    float toX = to.x;
    float toY = to.y;
    float fromX = from.x;
    float fromY = from.y;

    float xDiff = toX - fromX;
    float yDiff = toY - fromY;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    float xStepSize = xDiff / numberOfSteps;
    float yStepSize = yDiff / numberOfSteps;
    for (float i = 0.0; i < numberOfSteps; i++) {
        float x = fromX + (xStepSize * i);
        float y = fromY + (yStepSize * i);

        float red = colour_class.red;
        float green = colour_class.green;
        float blue = colour_class.blue;

        uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
        window.setPixelColour(round(x), round(y), colour);
    }
}

std::vector<CanvasPoint> Array_2DLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to) {

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
    std::cout << "number of steps: " << numberOfSteps << std::endl;
    for (float i = 0.0; i < numberOfSteps; i++) {
        float x = fromX + (xStepSize * i);
        float y = fromY + (yStepSize * i);

        result.push_back(CanvasPoint(x, y));

    }


    return result;
}

void drawTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour_class) {

    draw2DLine(window, CanvasPoint(triangle[0].x, triangle[0].y), CanvasPoint(triangle[1].x, triangle[1].y), colour_class);
    draw2DLine(window, CanvasPoint(triangle[1].x, triangle[1].y), CanvasPoint(triangle[2].x, triangle[2].y), colour_class);
    draw2DLine(window, CanvasPoint(triangle[2].x, triangle[2].y), CanvasPoint(triangle[0].x, triangle[0].y), colour_class);
}




void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour_class) {

    Colour white = Colour(255, 255, 255);
    //drawTriangle(window, triangle, white);

    CanvasPoint p0 = CanvasPoint(triangle[0].x, triangle[0].y);
    CanvasPoint pL = CanvasPoint(triangle[1].x, triangle[1].y);
    CanvasPoint p2 = CanvasPoint(triangle[2].x, triangle[2].y);

    // sort the points from top to bottom
    if (p0.y > p2.y) std::swap(p0, p2);
    if (p0.y > pL.y) std::swap(p0, pL);
    if (pL.y > p2.y) std::swap(pL, p2);

    float pR_x = p0.x - ((p2.x-p0.x)*(p0.y-pL.y))/(p2.y-p0.y);

    CanvasPoint pR = CanvasPoint(pR_x, pL.y);

    if (pL.x > pR.x) std::swap(pL, pR);

    std::cout << "p0: " << p0.x << " " << p0.y << std::endl;
    std::cout << "pL: " << pL.x << " " << pL.y << std::endl;
    std::cout << "pR: " << pR.x << " " << pR.y << std::endl;
    std::cout << "p2: " << p2.x << " " << p2.y << std::endl;

    draw2DLine(window, CanvasPoint(pL.x, pL.y), CanvasPoint(pR.x, pR.y), colour_class);

    //TOP HALF TRIANGLE

    // Interpolate p0 and pE
    std::vector<CanvasPoint> p0_pR = Array_2DLine(window, p0, pR);

    // Interpolate p0 and p1
    std::vector<CanvasPoint> p0_pL = Array_2DLine(window, p0, pL);

    std::cout << "p0_pR: " << p0_pR[0].x << " " << p0_pR[0].y << std::endl;
    std::cout << "p0_pL: " << p0_pL[0].x << " " << p0_pL[0].y << std::endl;
    std::cout << "p0_pR 1 : " << p0_pR[1].x << " " << p0_pR[1].y << std::endl;
    std::cout << "p0_pL 1 : " << p0_pL[1].x << " " << p0_pL[1].y << std::endl;

    int arrayLength = p0.y - pL.y;

    int j = 0;
    int jj = 0;

    for (float i = p0.y; i < pL.y ; i++) {
        std::cout << "p0_pL : " << p0_pL[j].x << " " << p0_pL[j].y << std::endl;
        std::cout << "p0_pR : " << p0_pR[jj].x << " " << p0_pR[jj].y << std::endl;
        std::cout << "iteration: " << i << std::endl;
        while (p0_pL[j].y < i && i<pL.y) {
            j++;
            std::cout << "while L: " << j << " " <<  p0_pL[j].y<< std::endl;

        }
        while (p0_pR[jj].y < i && i<pL.y) {
            jj++;
            std::cout << "while R " << jj << " " << p0_pR[jj].y << std::endl;

        }
        if (round((int)p0_pL[j].y) == i && round((int)p0_pR[jj].y) == i) {
            draw2DLine(window, CanvasPoint(p0_pL[j].x, i), CanvasPoint(p0_pR[jj].x, i), colour_class);

        }

    }


    // BOTTOM HALF TRIANGLE

    // Interpolate pR and p2
    std::vector<CanvasPoint> pR_p2 = Array_2DLine(window, pR, p2);

    // Interpolate pL and p2
    std::vector<CanvasPoint> pL_p2 = Array_2DLine(window, pL, p2);

    arrayLength = pL.y - p2.y;

    j = 0;
    jj = 0;

    for (float i = pL.y; i < p2.y ; i++) {

        while (pL_p2[j].y < i && i<p2.y) {
            j++;

        }
        while (pR_p2[jj].y < i && i<p2.y) {
            jj++;

        }
        if (round((int)pL_p2[j].y) == i && round((int)pR_p2[jj].y) == i) {
            draw2DLine(window, CanvasPoint(pL_p2[j].x, i), CanvasPoint(pR_p2[jj].x, i), colour_class);

        }

    }

    drawTriangle(window, triangle, white);
}


/*void loadMapTexture(DrawingWindow &window, CanvasTriangle triangle, CanvasTriangle textureTriangle) {

    TextureMap texture = TextureMap("texture.ppm");

    std::vector<std::vector<TexturePoint>> textureArray;

    //CanvasPoint textureArray[texture.width][texture.height];

    for (int i=0; i<texture.width; i++) {
        std::vector<TexturePoint> row;
        for (int j=0; j<texture.height; j++) {
            row.push_back(CanvasPoint(i, j));
            //textureArray[i][j] = texture.pixels[i+j*texture.width];
        }
    }

}*/

// Load cornell-box.mtl and populate colourPaletteMap global variable
void loadMtlFile(DrawingWindow &window) {
    std::ifstream file("cornell-box.mtl");
    std::string str;
    while (std::getline(file, str)) {
        std::vector<std::string> strings = split(str, ' ');
        if (strings[0]=="newmtl") {
            std::string colourName = strings[1];
            std::getline(file, str);
            std::vector<std::string> strings2 = split(str, ' ');
            std::string colourString = strings2[1];
            std::vector<std::string> colourStrings = split(colourString, '/');
            //Colour colour = Colour(std::stoi(colourStrings[0]), std::stoi(colourStrings[1]), std::stoi(colourStrings[2]));
            int r = std::stoi(colourStrings[0])*255;
            int g = std::stoi(colourStrings[1])*255;
            int b = std::stoi(colourStrings[2])*255;
            colourPalette.push_back(Colour(colourName, r, g, b));
        }
    }
    /*
    // Get an iterator pointing to the first element in the map
    std::map<std::string, <glm::vec3>>::iterator it = colourPaletteMap.begin();

    // Iterate through the map and print the elements
    while (it != colourPaletteMap.end())
    {
        std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
        ++it;
    }*/
}

// Load cornell-box.obj and read the vertices and faces
void loadObjFile(DrawingWindow &window) {

    std::ifstream file("cornell-box.obj");
    std::string str;
    std::vector<ModelTriangle> faces;
    std::vector<glm::vec3> vertices;

    while (std::getline(file, str)) {

        //std::cout << str << std::endl;
        std::vector<std::string> strings = split(str, ' ');

        if (strings[0]=="v") {
            vertices.push_back(glm::vec3(std::stof(strings[1])*0.35, std::stof(strings[2])*0.35, std::stof(strings[3])*0.35));
            std::cout << vertices[0][0] << std::endl;
        }
        if (strings[0]=="f") {
            ModelTriangle triangle = ModelTriangle(vertices[std::stoi(split(strings[1], '/')[0])-1], vertices[std::stoi(split(strings[2], '/')[0])-1], vertices[std::stoi(split(strings[3], '/')[0])-1], Colour(255, 255, 255));
            faces.push_back(triangle);
            //std::cout << faces[0].vertices[0][0] << std::endl;
        }
    }
    int i=0;
    for (ModelTriangle triangle : faces) {
        std::cout << "Triangle: " << i << std::endl;
        std::cout << triangle << std::endl;
        i++;
    }
}


void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
        else if (event.key.keysym.sym == SDLK_u) {
            std::cout << "u" << std::endl;

            CanvasPoint p1 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p2 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p3 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);

            Colour colour = Colour(rand()%256, rand()%256, rand()%256);

            drawTriangle(window, CanvasTriangle(p1, p2, p3), colour);

        }
        else if (event.key.keysym.sym == SDLK_f) {
            std::cout << "f" << std::endl;

            CanvasPoint p1 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p2 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);
            CanvasPoint p3 = CanvasPoint(rand()%WIDTH+1, rand()%HEIGHT+1);

            Colour colour = Colour(rand()%256, rand()%256, rand()%256);

            drawFilledTriangle(window, CanvasTriangle(p1, p2, p3), colour);

        } else if (event.key.keysym.sym == SDLK_l) {
            //loadMtlFile(window);
            loadObjFile(window);
        }
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {

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

	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

    while (true) {
    // We MUST poll for events - otherwise the window will freeze !
    if (window.pollForInputEvents(event)) handleEvent(event, window);

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
