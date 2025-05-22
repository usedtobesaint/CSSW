#include "gui.h"
#include "parser.h"
#include <thread>
#include <iostream>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

using namespace gui;

// Function to recursively render the tree using ImGui
void RenderTree(prsr::Node* node, int depth = 0) {
    if (!node) return;

    // Create a unique label for each node using its value and address
    std::string label = node->value + "##" + std::to_string(reinterpret_cast<uintptr_t>(node));

    if (node->isOperator) {
        if (ImGui::TreeNode(label.c_str(), "%s", node->value.c_str())) {
            for (auto child : node->children) {
                RenderTree(child, depth + 1);
            }
            ImGui::TreePop();
        }
    } else {
        ImGui::BulletText("%s", node->value.c_str());
    }
}

void PrintTree(prsr::Node* node, int depth = 0) {
    if (!node) return;
    for (int i = 0; i < depth; ++i) std::cout << "  ";
    std::cout << node->value << (node->isOperator ? " (op)" : "") << std::endl;
    for (auto child : node->children) {
        PrintTree(child, depth + 1);
    }
}

bool showShapesWindow = false;

prsr::Node* treeRoot = nullptr; // To store the parse tree root

void ImGuiPrintTree(prsr::Node* node, int depth = 0) {
    if (!node) return;
    ImGui::Indent(depth * 20.0f); // Indent for visual hierarchy
    if (node->isOperator) {
        ImGui::Text("%s (op)", node->value.c_str());
    } else {
        ImGui::Text("%s", node->value.c_str());
    }
    for (auto child : node->children) {
        ImGuiPrintTree(child, depth + 1);
    }
    ImGui::Unindent(depth * 20.0f);
}

std::string FullySimplifyAndCorrect(std::string expr) {
    std::string prev;
    int maxIterations = 40;
    int iter = 0;
    do {
        prev = expr;
        // 1. Simplify
        expr = prsr::simplifyExpression(expr);
        // 2. Check for errors
        prsr::errors.clear();
        prsr::checkExpression(expr.c_str());
        // 3. If errors, correct
        if (!prsr::errors.empty()) {
            expr = prsr::correctExpression(&expr[0], prsr::errors);
        }
        iter++;
    } while ((expr != prev || !prsr::errors.empty()) && iter < maxIterations);
    return expr;
}

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
        ImGui::SetNextWindowSize({ 1000, 1100 });
        ImGui::Begin(
            "Parser",
            &isRunning,
            ImGuiWindowFlags_NoCollapse
        );

        ImGui::InputText("Write your expression here", prsr::expression, IM_ARRAYSIZE(prsr::expression),
            ImGuiInputTextFlags_CharsUppercase |
            ImGuiInputTextFlags_CharsNoBlank);
        if (ImGui::Button("Auto-correct & Simplify")) {
            std::string input = prsr::expression;
            std::string result = FullySimplifyAndCorrect(input);
            prsr::simplifiedExpression = result;
            prsr::errors.clear();
            prsr::checkExpression(result.c_str());
        }
        if (ImGui::Button("Model system")) {
            prsr::modelSystem(prsr::simplifiedExpression, 6);
        }
        ImGui::Text("Final expression: %s", prsr::simplifiedExpression.c_str());
        prsr::displayErrors(prsr::errors);

        ImGui::Checkbox("Optimize expression", &showShapesWindow);

        // If the checkbox is checked, show the shapes window
        if (showShapesWindow) {
            // Optimize the expression
            prsr::optimizedExpression = prsr::optimizeExpression(prsr::simplifiedExpression);

            // Clean up the previous tree if it exists
            if (treeRoot) {
                delete treeRoot;
                treeRoot = nullptr;
            }

            // Build and optimize the parse tree
            treeRoot = prsr::buildParseTree(prsr::optimizedExpression);
            treeRoot = prsr::optimizeParallelTree(treeRoot);
            

            // Display the tree structure using ImGui
            ImGui::Separator();
            ImGui::Text("Parse Tree:");
            if (treeRoot) {
                ImGuiPrintTree(treeRoot);
            }
            else {
                ImGui::Text("No tree to display.");
            }
        }

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