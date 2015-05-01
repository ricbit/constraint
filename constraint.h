#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <set>
#include <cstdio>
#include <limits>
#include <queue>

struct Variable {
  int lmin, lmax;
  int id;
};

struct Constraint {
  int lmin, lmax;
  std::set<int> variables;
};

class ExternalConstraint {
 public:
  virtual bool operator()(const std::vector<Variable>& variables) const = 0;
};

class ConstraintSolver {
  int recursion_nodes;
  std::vector<Variable> variables, solution;
  std::vector<Constraint> constraints;
  std::vector<const ExternalConstraint*> external;
 public:
  ConstraintSolver() : recursion_nodes(0) {}

  int create_variable(int lmin, int lmax) {
    Variable v;
    v.lmin = lmin;
    v.lmax = lmax;
    v.id = variables.size();
    variables.push_back(v);
    return variables.size() - 1;
  }

  void add_external_constraint(const ExternalConstraint* cons) {
    external.push_back(cons);
  }

  const Variable& value(int id) {
    return solution[id];
  }

  int create_constraint(int lmin, int lmax) {
    Constraint cons;
    cons.lmin = lmin;
    cons.lmax = lmax;
    constraints.push_back(cons);
    return constraints.size() - 1;
  }

  void add_variable(int constraint_id, int variable_id) {
    constraints[constraint_id].variables.insert(variable_id);
  }

  void solve() {
    tight();
    recursion();
    std::cout << "Recursion nodes: " << recursion_nodes << "\n";
  }

  bool recursion() {
    recursion_nodes++;
    if (finished()) {      
      solution = variables;
      return true;
    }
    int index = choose();
    std::vector<Variable> bkp = variables;
    auto var = variables[index];
    for (int i = var.lmin; i <= var.lmax; i++) {
      variables = bkp;
      auto& var = variables[index];
      var.lmin = var.lmax = i;
      if (tight() && valid()) {
        if (recursion()) {
          return true;
        }
      }
    }
    variables = bkp;
    return false;
  }

  bool valid() {
    for (auto cons : constraints) {
      int cmin = 0, cmax = 0;
      for (auto i : cons.variables) {
        auto var = variables[i];
        cmin += var.lmin;
        cmax += var.lmax;
      }
      if (cmax < cons.lmin || cmin > cons.lmax) {
        return false;
      }
    }
    for (auto& cons : external) {
      if (!(*cons)(variables)) {
        return false;
      }
    }
    return true;
  }

  int choose() {
    int chosen = 0;
    int diff = std::numeric_limits<int>::max();
    for (const auto& var : variables) {
      if (var.lmin != var.lmax) {
        if (var.lmax - var.lmin < diff) {
          chosen = var.id;
          diff = var.lmax - var.lmin;
        }
      }
    }
    return chosen;
  }

  bool finished() {
    for (auto var : variables) {
      if (var.lmin != var.lmax) {
        return false;
      }
    }
    return true;
  }

  bool tight() {
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto cons : constraints) {
        for (auto ivar : cons.variables) {
          auto& var = variables[ivar];
          // increase min
          int limit = cons.lmin;
          for (auto iother : cons.variables) {
            auto& other = variables[iother];
            if (ivar != iother) {
              limit -= other.lmax;
            }
          }
          if (limit > var.lmax) {
            return false;
          }
          if (var.lmin < limit) {
            var.lmin = limit;
            changed = true;
          }
          // decrease max
          limit = cons.lmax;
          for (auto iother : cons.variables) {
            auto& other = variables[iother];
            if (ivar != iother) {
              limit -= other.lmin;
            }
          }
          if (limit < var.lmin) {
            return false;
          }
          if (var.lmax > limit) {
            var.lmax = limit;
            changed = true;
          }
        }
      }
    }
    return true;
  }
};

