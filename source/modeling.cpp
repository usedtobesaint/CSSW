#include "parser.h"
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <set>
#include <functional>

struct TaskAssignment {
    int proc;
    int startTime;
    int endTime;
    std::string op;
};

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
        if (child->isOperator) {
            prsr::Node* opChild = buildTaskGraph(child);
            if (opChild) node->children.push_back(opChild);
        } else {
            // Додаємо "заглушку" для листа, щоб зберегти залежність
            node->children.push_back(nullptr);
        }
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
            if (!child) continue;
            q.push({child, lvl + 1});
        }
    }
    return levels;
}

// Функція для визначення тривалості операції
int getOpDuration(const std::string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*") return 2;
    if (op == "/") return 4;
    return 1;
}

// Повністю готова функція: планування з урахуванням залежностей (без buildTaskGraph)
std::vector<TaskAssignment> assignTasksWithDependencies(prsr::Node* root, int procCount) {
    std::vector<TaskAssignment> assignments;
    if (!root) return assignments;
    std::vector<int> procAvailable(procCount, 0);
    struct TaskInfo {
        prsr::Node* node;
        int start;
        int end;
        int proc;
    };
    std::vector<TaskInfo> taskInfos;
    // DFS: для бінарних операторів (2 дитини)
    std::function<int(prsr::Node*)> dfs = [&](prsr::Node* node) -> int {
        if (!node) return 0;
        if (!node->isOperator) return 0; // лист готовий одразу
        int leftFinish = 0, rightFinish = 0;
        if (node->children.size() > 0) leftFinish = dfs(node->children[0]);
        if (node->children.size() > 1) rightFinish = dfs(node->children[1]);
        int earliestStart = std::max(leftFinish, rightFinish);
        int minProc = 0;
        int minTime = std::max(procAvailable[0], earliestStart);
        for (int i = 1; i < procCount; ++i) {
            int t = std::max(procAvailable[i], earliestStart);
            if (t < minTime) {
                minTime = t;
                minProc = i;
            }
        }
        int duration = getOpDuration(node->value);
        int start = std::max(procAvailable[minProc], earliestStart);
        int end = start + duration;
        taskInfos.push_back({node, start, end, minProc});
        procAvailable[minProc] = end;
        return end;
    };
    dfs(root);
    std::sort(taskInfos.begin(), taskInfos.end(), [](const TaskInfo& a, const TaskInfo& b) {
        return a.start < b.start;
    });
    for (const auto& t : taskInfos) {
        assignments.push_back({t.proc, t.start, t.end, t.node->value});
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
    // Масив кількостей процесорів для замірів
    std::vector<int> procVariants = {1, 2, 5, 6, 8, 10};
    for (size_t i = 0; i < procVariants.size(); ++i) {
        int pCount = procVariants[i];
        std::cout << "\n=== Моделювання для " << pCount << " процесорів ===" << std::endl;
        auto assignments = assignTasksWithDependencies(tree, pCount);
        auto levels = groupByLevels(tree);
        int seqTime = levels.size();
        int parTime = assignments.empty() ? 0 : assignments.back().endTime;
        std::set<int> usedProcSet;
        for (const auto& t : assignments) usedProcSet.insert(t.proc);
        int usedProcs = usedProcSet.size();
        computeMetrics(seqTime, parTime, usedProcs, pCount);
        if (i == procVariants.size() - 1) {
            printGanttTable(assignments, pCount);
        }
    }
    // Clean up
    if (tree) delete tree;
}
