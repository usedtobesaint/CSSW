#include "gui.h"
#include "parser.h"
#include <thread>
#include <iostream>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

using namespace gui;

// Function to recursively render the tree using ImGui
void RenderTree(prsr::Node* node) {
    if (!node) return;

    if (node->isOperator) {
        // For operators, create a tree node with the operator value
        if (ImGui::TreeNode(node, "%s", node->value.c_str())) {
            // Recursively render children
            for (auto child : node->children) {
                RenderTree(child);
            }
            ImGui::TreePop();
        }
    }
    else {
        // For leaf nodes (variables or numbers), display as a non-expandable item
        ImGui::BulletText("%s", node->value.c_str());
    }
}

bool showShapesWindow = false;
prsr::Node* treeRoot = nullptr; // To store the parse tree root

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
    gui::CreateImGui(parserWindow, parserDevice, parserContext);
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
        }

        ImGui::Text("Corrected expression: %s", prsr::correctedExpression.c_str());

        ImGui::Checkbox("Optimize expression", &showShapesWindow);

        // If the checkbox is checked, show the shapes window
        if (showShapesWindow) {
            // Optimize the expression
            prsr::optimizedExpression = prsr::optimizeExpression(prsr::correctedExpression);

            // Clean up the previous tree if it exists
            if (treeRoot) {
                delete treeRoot;
                treeRoot = nullptr;
            }

            // Build and optimize the parse tree
            treeRoot = prsr::buildParseTree(prsr::optimizedExpression);
            treeRoot = prsr::optimizeParallelTree(treeRoot);

            // Display the tree as a string for reference
            std::string treeString = prsr::treeToString(treeRoot);
            ImGui::Text("Tree as string: %s", treeString.c_str());

            // Display the tree structure using ImGui
            ImGui::Separator();
            ImGui::Text("Parse Tree:");
            if (treeRoot) {
                RenderTree(treeRoot);
            }
            else {
                ImGui::Text("No tree to display.");
            }
        }

        ImGui::Text("Optimized expression: %s", prsr::optimizedExpression.c_str());

        ImGui::End();

        gui::EndRender(parserDevice, parserContext);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Clean up the tree before exiting
    if (treeRoot) {
        delete treeRoot;
        treeRoot = nullptr;
    }

    // Clean up resources
    gui::DestroyImGui(parserContext);
    gui::DestroyDevice(parserDevice);
    gui::DestroyHWindow(parserWindow);

    return EXIT_SUCCESS;
}