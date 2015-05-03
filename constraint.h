#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <set>
#include <cstdio>
#include <limits>
#include <queue>

struct Variable {
  bool fixed;
  int lmin, lmax;
  int id;
  std::vector<int> constraints;
  Variable() : fixed(false) {}
};

struct Constraint {
  int lmin, lmax;
  std::vector<int> variables;
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

  void change_var(int var_id, int lmin, int lmax) {
    variables[var_id].lmin = lmin;
    variables[var_id].lmax = lmax;
    variables[var_id].fixed = lmin == lmax;
  }

  int create_variable(int lmin, int lmax) {
    Variable v;
    v.lmin = lmin;
    v.lmax = lmax;
    v.fixed = lmin == lmax;
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
    constraints[constraint_id].variables.push_back(variable_id);
    variables[variable_id].constraints.push_back(constraint_id);
  }

  void solve() {
    std::cout << "Variables: " << variables.size() << "\n";
    std::cout << "Constraints: " << constraints.size() << "\n";
    tight();
    int freevars = 0;
    for (const auto& var : variables) {
      if (!var.fixed) {
        freevars++;
      }
    }
    std::cout << "Free variables: " << freevars << "\n";
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
      change_var(index, i, i);
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
      if (!var.fixed) {
        if (var.lmax - var.lmin < diff) {
          chosen = var.id;
          diff = var.lmax - var.lmin;
        } else if (var.lmax - var.lmin == diff && 
            var.constraints.size() > variables[chosen].constraints.size()) {
          chosen = var.id;
        }
      }
    }
    return chosen;
  }

  bool finished() {
    for (auto var : variables) {
      if (!var.fixed) {
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
            change_var(ivar, limit, var.lmax);
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
            change_var(ivar, var.lmin, limit);
            changed = true;
          }
        }
      }
    }
    return true;
  }
};

