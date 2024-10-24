#include "gui.h"
#include "parser.h"
#include <thread>
#include <iostream>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

using namespace gui;


int main(int argc, char* argv[])
{

	// Create gui
	gui::CreateHWindow("CSSW", "Parser");
	gui::CreateDevice();
	gui::CreateImGui();
	while (gui::isRunning)
	{
		gui::BeginRender();
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ 800, 500 });
		ImGui::Begin(
			"Parser",
			&isRunning,
			ImGuiWindowFlags_NoResize |
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





		ImGui::End();
		gui::EndRender();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return EXIT_SUCCESS;
}