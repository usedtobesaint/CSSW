#include "parser.h"
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <set>

// 1. Перевірка валідності виразу
bool validateExpression(const std::string& expr) {
    prsr::errors.clear();
    prsr::checkExpression(expr.c_str());
    return prsr::errors.empty();
}

// 2. Побудова дерева та оптимізація
prsr::Node* buildOptimizedTree(const std::string& expr) {
    std::string simplified = prsr::simplifyExpression(expr);
    return prsr::buildParseTree(simplified);
}

// 3. Побудова графу задачі (залишаємо лише оператори)
prsr::Node* buildTaskGraph(prsr::Node* root) {
    if (!root) return nullptr;
    if (!root->isOperator) return nullptr; // Лист — не операція
    prsr::Node* node = new prsr::Node(root->value, true, false, false);
    for (auto* child : root->children) {
        prsr::Node* opChild = buildTaskGraph(child);
        if (opChild) node->children.push_back(opChild);
    }
    return node;
}

// 4. Групування операцій за рівнями (BFS)
std::vector<std::vector<prsr::Node*>> groupByLevels(prsr::Node* root) {
    std::vector<std::vector<prsr::Node*>> levels;
    if (!root) return levels;
    std::queue<std::pair<prsr::Node*, int>> q;
    q.push({root, 0});
    while (!q.empty()) {
        auto [node, lvl] = q.front(); q.pop();
        if (levels.size() <= lvl) levels.push_back({});
        levels[lvl].push_back(node);
        for (auto* child : node->children) {
            q.push({child, lvl + 1});
        }
    }
    return levels;
}

// 5. Розподіл тасок між процесорами (лінійка)
struct TaskAssignment {
    int proc;
    int startTime;
    int endTime;
    std::string op;
};
std::vector<TaskAssignment> assignTasks(const std::vector<std::vector<prsr::Node*>>& levels, int procCount) {
    std::vector<TaskAssignment> assignments;
    int time = 0;
    for (const auto& level : levels) {
        int p = 0;
        for (auto* node : level) {
            assignments.push_back({p % procCount, time, time + 1, node->value});
            p++;
        }
        time++;
    }
    return assignments;
}

// 6. Метрики
void computeMetrics(int seqTime, int parTime, int usedProcs, int totalProcs) {
    double speedup = (double)seqTime / parTime;
    double effActive = speedup / usedProcs;
    double effTotal = speedup / totalProcs;
    std::cout << "Sequential computation time: " << seqTime << std::endl;
    std::cout << "Parallel computation time: " << parTime << std::endl;
    std::cout << "Speedup: " << speedup << std::endl;
    std::cout << "Active processors used: " << usedProcs << std::endl;
    std::cout << "Total processors: " << totalProcs << std::endl;
    std::cout << "Efficiency (active): " << effActive << std::endl;
    std::cout << "Efficiency (total): " << effTotal << std::endl;
}

// 7. Візуалізація діаграми Ганта (текстова)
void printGantt(const std::vector<TaskAssignment>& assignments, int procCount) {
    std::map<int, std::vector<std::string>> gantt;
    for (const auto& t : assignments) {
        while (gantt[t.proc].size() < t.startTime) gantt[t.proc].push_back(" ");
        gantt[t.proc].push_back(t.op);
    }
    for (int p = 0; p < procCount; ++p) {
        std::cout << "P" << p+1 << ": ";
        for (const auto& op : gantt[p]) std::cout << "[" << op << "]";
        std::cout << std::endl;
    }
}

void printGanttTable(const std::vector<TaskAssignment>& assignments, int procCount) {
    // Знаходимо максимальний час
    int maxTime = 0;
    for (const auto& t : assignments) {
        if (t.endTime > maxTime) maxTime = t.endTime;
    }
    // Створюємо таблицю: procCount x maxTime
    std::vector<std::vector<std::string>> table(procCount, std::vector<std::string>(maxTime, "   "));
    for (const auto& t : assignments) {
        for (int tstep = t.startTime; tstep < t.endTime; ++tstep) {
            table[t.proc][tstep] = " " + t.op + " ";
        }
    }
    // Вивід заголовка
    std::cout << "    ";
    for (int t = 0; t < maxTime; ++t) std::cout << "| " << t << " ";
    std::cout << "|\n";
    for (int p = 0; p < procCount; ++p) {
        std::cout << "P" << (p+1) << " ";
        if (p+1 < 10) std::cout << " "; // вирівнювання
        for (int t = 0; t < maxTime; ++t) {
            std::cout << "|" << table[p][t];
        }
        std::cout << "|\n";
    }
}

// === Основна функція ===
void prsr::modelSystem(const std::string& expr, int procCount) {
    // 1. Перевірка
    if (!validateExpression(expr)) {
        std::cout << "Error: the expression is not valid!" << std::endl;
        return;
    }
    // 2. Дерево
    prsr::Node* tree = buildOptimizedTree(expr);
    tree = prsr::optimizeParallelTree(tree);
    if (!tree) {
        std::cout << "Error: failed to build the tree!" << std::endl;
        return;
    }
    // 3. Граф задачі
    prsr::Node* taskGraph = buildTaskGraph(tree);
    if (!taskGraph) {
        std::cout << "The expression does not contain operations for the task graph." << std::endl;
        return;
    }
    // 4. Групування
    auto levels = groupByLevels(taskGraph);
    // 5. Розподіл
    auto assignments = assignTasks(levels, procCount);
    // 6. Метрики
    int seqTime = levels.size();
    int parTime = assignments.empty() ? 0 : assignments.back().endTime;
    std::set<int> usedProcSet;
    for (const auto& t : assignments) usedProcSet.insert(t.proc);
    int usedProcs = usedProcSet.size();
    computeMetrics(seqTime, parTime, usedProcs, procCount);
    // 7. Діаграма Ганта
    printGanttTable(assignments, procCount);

    // Clean up
    delete tree;
    delete taskGraph;
}
