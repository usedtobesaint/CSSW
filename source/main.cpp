#include "gui.h"
#include "parser.h"
#include <thread>
#include <iostream>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

using namespace gui;

bool showShapesWindow = false;

int main(int argc, char* argv[])
{

	// Create gui
	gui::CreateHWindow("Parser", parserWindow);
	if (!parserWindow) {
		std::cout << "Failed to create window." << std::endl;
		return 0;
	}

	gui::CreateDevice(parserWindow, parserDevice);

	if (!gui::CreateDevice(parserWindow, parserDevice)) {
		std::cerr << "Failed to create device" << std::endl;
		return EXIT_FAILURE;
	}
	gui::CreateImGui(parserWindow, parserDevice,parserContext);
	while (gui::isRunning)
	{
		gui::BeginRender(parserContext);
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ 800, 500 });
		ImGui::Begin(
			"Parser",
			&isRunning,
			ImGuiWindowFlags_NoCollapse

		);


		ImGui::InputText("Write your expression here", prsr::expression, IM_ARRAYSIZE(prsr::expression),
			ImGuiInputTextFlags_CharsUppercase |
			ImGuiInputTextFlags_CharsNoBlank);
		if (ImGui::Button("Check")) {
			prsr::errors.clear();
			prsr::checkExpression(prsr::expression);
		}
		prsr::displayErrors(prsr::errors);

		if (ImGui::Button("Correct")) {
			prsr::correctedExpression = prsr::correctExpression(prsr::expression, prsr::errors);
			if (prsr::tree != nullptr) {
				delete prsr::tree; // Ensure we release previous memory if the tree is dynamically allocated
				prsr::tree = nullptr;
			}
			prsr::tree = prsr::parseExpression(prsr::correctedExpression); // Parse and store tree once
			std::cout << "Parsed Expression Tree: " << prsr::correctedExpression << std::endl;
		}

		ImGui::Text("Corrected expression: %s", prsr::correctedExpression.c_str());

		ImGui::Checkbox("Shapes", &showShapesWindow);

		ImGui::End();

		// If the checkbox is checked, show the shapes window
		if (showShapesWindow && prsr::tree != nullptr) {
			ImGui::Begin("Shapes Window", &showShapesWindow);  // Window will close if the checkbox is unchecked
			prsr::drawNode(prsr::tree);
			ImGui::End();
		}

		gui::EndRender(parserDevice, parserContext);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Clean up resources
	gui::DestroyImGui(parserContext);
	gui::DestroyDevice(parserDevice);
	gui::DestroyHWindow(parserWindow);

	return EXIT_SUCCESS;
}