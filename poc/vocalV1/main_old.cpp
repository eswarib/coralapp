#include "transcriber.h"
#include "textInjector.h"
#include "recorder.h"
#include <chrono>
#include <thread>
#include <iostream>

int main() {
//    redirectOutput();
    while (true) {
	std::cout << "Recording Audio... "<< std::endl;
        //recordAudio("input.wav");
	recorder::getInstance()->recordAudio("input.wav");
	std::cout << "Transcribing Audio... "<< std::endl;
        //std::string text = transcribeAudio("input.wav");
        const std::string text = transcriber::getInstance()->transcribeAudio("input.wav");
	std::cout << "Typing the text... "<< std::endl;
          
	//typeText(text);
        textInjector::getInstance()->typeText(text);	
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
} 