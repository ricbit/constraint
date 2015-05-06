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

struct Metadata {
  int id;
  std::vector<int> constraints;
};

struct Constraint {
  int lmin, lmax;
  int id;
  std::vector<int> variables;
};

class State {
  std::vector<Bounds> bounds, solution;
  std::vector<Metadata> metadata;
 public:
  State(const std::vector<Variable>& variables) 
      : bounds(variables.size()), metadata(variables.size()) {
    for (const Variable& var : variables) {
      bounds[var.id].lmin = var.lmin;
      bounds[var.id].lmax = var.lmax;
      metadata[var.id].id = var.id;
      metadata[var.id].constraints = var.constraints;
    }
  }

  void save_solution() {
    solution = bounds;
  }

  int read_lmax(int id) const {
    return bounds[id].lmax;
  }

  int read_lmin(int id) const {
    return bounds[id].lmin;
  }

  bool fixed(int id) const {
    return bounds[id].lmin == bounds[id].lmax;
  }

  int value(int id) const {
    return solution[id].lmin;
  }

  void change_var(int var_id, int lmin, int lmax) {
    bounds[var_id].lmin = lmin;
    bounds[var_id].lmax = lmax;
  }

  const std::vector<Bounds>& get_variables() {
    return bounds;
  }

  void set_variables(const std::vector<Bounds>& new_vars) {
    bounds = new_vars;
  }
};

class ExternalConstraint {
 public:
  virtual bool operator()(const State* state) const = 0;
};

class ConstraintSolver {
  int recursion_nodes, constraints_checked;
  State* state;
  std::vector<Variable> variables;
  std::vector<Constraint> constraints;
  std::vector<const ExternalConstraint*> external;
  std::queue<int> active_constraints;
  std::vector<bool> queued_constraints;
 public:
  ConstraintSolver() 
      : recursion_nodes(0), constraints_checked(0), state(nullptr) {}
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

  int value(int id) {
    return state->value(id);
  }

  int create_constraint(int lmin, int lmax) {
    Constraint cons;
    cons.lmin = lmin;
    cons.lmax = lmax;
    cons.id = constraints.size();
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
    queued_constraints.resize(constraints.size(), true);
    for (const Constraint& cons : constraints) {
      active_constraints.push(cons.id);
    }
    tight();
    int freevars = 0;
    for (const auto& var : variables) {
      if (!fixed(var.id)) {
        freevars++;
      }
      //std::cout << "var " << var.id << " : lmin " << read_lmin(var.id)
      //          << " lmax " << read_lmax(var.id) << "\n";
    }
    /*for (const Constraint& cons : constraints) {
      std::cout << "cons " << cons.id << " : ";
      for (int var : cons.variables) {
        std::cout << var << " ";
      }
      std::cout << "\n";
    }*/
    std::cout << "Free variables: " << freevars << "\n";
    recursion();
    std::cout << "Recursion nodes: " << recursion_nodes << "\n";
    std::cout << "Constraints checked: " << constraints_checked << "\n";
  }

  bool recursion() {
    recursion_nodes++;
    if (finished()) {      
      state->save_solution();
      return true;
    }
    int index = choose();
    std::vector<Bounds> bkp = state->get_variables();
    int savemin = read_lmin(index), savemax = read_lmax(index);
    for (int i = savemin; i <= savemax; i++) {
      state->set_variables(bkp);
      change_var(index, i, i);
      for (int cons : variables[index].constraints) {
        if (!queued_constraints[cons]) {
          active_constraints.push(cons);
          queued_constraints[cons] = true;
        }
      }
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
    bool valid = true;
    while (!active_constraints.empty()) {
      int id = active_constraints.front();
      active_constraints.pop();
      queued_constraints[id] = false;
      if (update_constraint(constraints[id], valid)) {
        for (int var : constraints[id].variables) {
          for (int cons : variables[var].constraints) {
            if (!queued_constraints[cons]) {
              active_constraints.push(cons);
              queued_constraints[cons] = true;
            }
          }
        }
      }
      if (!valid) {
        while (!active_constraints.empty()) {
          int id = active_constraints.front();
          active_constraints.pop();
          queued_constraints[id] = false;
        }
        return false;
      }
    }
    return true;
  }

  bool update_constraint(const Constraint& cons, bool& valid) {
    constraints_checked++;
    int allmax = 0, allmin = 0;
    for (int ivar : cons.variables) {
      allmax += read_lmax(ivar);
      allmin += read_lmin(ivar);
    }
    if (allmax < cons.lmin || allmin > cons.lmax) {
      valid = false;
      return false;
    }
    for (int ivar : cons.variables) {
      // increase min
      int limit = cons.lmin - allmax + read_lmax(ivar);
      if (limit > read_lmax(ivar)) {
        valid = false;
        return false;
      }
      if (read_lmin(ivar) < limit) {
        change_var(ivar, limit, read_lmax(ivar));
        return true;
      }
      // decrease max
      limit = cons.lmax - allmin + read_lmin(ivar);
      if (limit < read_lmin(ivar)) {
        valid = false;
        return false;
      }
      if (read_lmax(ivar) > limit) {
        change_var(ivar, read_lmin(ivar), limit);
        return true;
      }
    }
    return false;
  }
};

