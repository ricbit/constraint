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
  std::vector<int> constraints;
};

struct Bounds {
  int lmin, lmax;
};

struct Constraint {
  int lmin, lmax;
  std::vector<int> variables;
};

class State {
  std::vector<Variable> variables, solution;
 public:
  State(std::vector<Variable>& variables_) : variables(variables_) {}

  void save_solution() {
    solution = variables;
  }

  int read_lmax(int id) const {
    return variables[id].lmax;
  }

  int read_lmin(int id) const {
    return variables[id].lmin;
  }

  bool fixed(int id) const {
    return variables[id].lmin == variables[id].lmax;
  }

  const Variable& value(int id) const {
    return solution[id];
  }

  void change_var(int var_id, int lmin, int lmax) {
    variables[var_id].lmin = lmin;
    variables[var_id].lmax = lmax;
  }

  const std::vector<Variable>& get_variables() {
    return variables;
  }

  void set_variables(const std::vector<Variable>& new_vars) {
    variables = new_vars;
  }
};

class ExternalConstraint {
 public:
  virtual bool operator()(const State* state) const = 0;
};

class ConstraintSolver {
  int recursion_nodes;
  State* state;
  std::vector<Variable> variables;
  std::vector<Constraint> constraints;
  std::vector<const ExternalConstraint*> external;
 public:
  ConstraintSolver() : recursion_nodes(0), state(nullptr) {}
  ~ConstraintSolver() { 
    delete state;
  }

  int read_lmax(int id) {
    return state->read_lmax(id);
  }

  int read_lmin(int id) {
    return state->read_lmin(id);
  }

  bool fixed(int id) {
    return state->fixed(id);
  }

  void change_var(int var_id, int lmin, int lmax) {
    state->change_var(var_id, lmin, lmax);
  }

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
    return state->value(id);
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
    state = new State(variables);
    std::cout << "Variables: " << variables.size() << "\n";
    std::cout << "Constraints: " << constraints.size() << "\n";
    tight();
    int freevars = 0;
    for (const auto& var : variables) {
      if (!fixed(var.id)) {
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
      state->save_solution();
      return true;
    }
    int index = choose();
    std::vector<Variable> bkp = state->get_variables();
    int savemin = read_lmin(index), savemax = read_lmax(index);
    for (int i = savemin; i <= savemax; i++) {
      state->set_variables(bkp);
      change_var(index, i, i);
      if (tight() && valid()) {
        if (recursion()) {
          return true;
        }
      }
    }
    state->set_variables(bkp);
    return false;
  }

  bool valid() {
    for (const Constraint& cons : constraints) {
      int cmin = 0, cmax = 0;
      for (int i : cons.variables) {
        cmin += read_lmin(i);
        cmax += read_lmax(i);
      }
      if (cmax < cons.lmin || cmin > cons.lmax) {
        return false;
      }
    }
    for (auto& cons : external) {
      if (!(*cons)(state)) {
        return false;
      }
    }
    return true;
  }

  int choose() {
    int chosen = 0;
    int diff = std::numeric_limits<int>::max();
    for (const Variable& var : variables) {
      if (!fixed(var.id)) {
        int cur_diff = read_lmax(var.id) - read_lmin(var.id);
        if (cur_diff < diff) {
          chosen = var.id;
          diff = cur_diff;
        } else if (cur_diff == diff && 
            var.constraints.size() > variables[chosen].constraints.size()) {
          chosen = var.id;
        }
      }
    }
    return chosen;
  }

  bool finished() {
    for (const Variable& var : variables) {
      if (!fixed(var.id)) {
        return false;
      }
    }
    return true;
  }

  bool tight() {
    bool changed = true;
    while (changed) {
      changed = false;
      for (const Constraint& cons : constraints) {
        for (int ivar : cons.variables) {
          auto& var = variables[ivar];
          // increase min
          int limit = cons.lmin;
          for (int iother : cons.variables) {
            auto& other = variables[iother];
            if (ivar != iother) {
              limit -= read_lmax(iother);
            }
          }
          if (limit > read_lmax(ivar)) {
            return false;
          }
          if (read_lmin(ivar) < limit) {
            change_var(ivar, limit, read_lmax(ivar));
            changed = true;
          }
          // decrease max
          limit = cons.lmax;
          for (auto iother : cons.variables) {
            auto& other = variables[iother];
            if (ivar != iother) {
              limit -= read_lmin(iother);
            }
          }
          if (limit < read_lmin(ivar)) {
            return false;
          }
          if (read_lmax(ivar) > limit) {
            change_var(ivar, read_lmin(ivar), limit);
            changed = true;
          }
        }
      }
    }
    return true;
  }
};

