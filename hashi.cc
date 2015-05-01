#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <set>
#include <cstdio>
#include <limits>
#include <queue>

using namespace std;

struct Node {
  int x, y;
  int size;
  int id;
  set<int> links;
  Node(int x_, int y_, int size_, int id_)
      : x(x_), y(y_), size(size_), id(id_) {}
};

struct Link {
  int a, b;
  bool horizontal;
  int id;
  set<int> forbidden;
  Link(int a_, int b_, bool horizontal_, int id_)
      : a(a_), b(b_), horizontal(horizontal_), id(id_) {}
};

struct Variable {
  int lmin, lmax;
  int id;
};

struct Constraint {
  int lmin, lmax;
  set<int> variables;
};

class ExternalConstraint {
 public:
  virtual bool operator()(const vector<Variable>& variables) const = 0;
};

class SingleGroupConstraint : public ExternalConstraint {
  const vector<Node>& nodes;
  const vector<Link>& links;
 public:
  SingleGroupConstraint(const vector<Node>& nodes_, const vector<Link>& links_)
      : nodes(nodes_), links(links_) {}

  virtual bool operator()(const vector<Variable>& variables) const {
    vector<bool> visited(nodes.size(), false);
    queue<int> next;
    next.push(0);
    visited[0] = true;
    while (!next.empty()) {
      int cur = next.front();
      next.pop();
      for (const auto& ilink : nodes[cur].links) {
        const auto& link = links[ilink];
        int other = link.a == cur ? link.b : link.a;
        if (!visited[other] && variables[ilink].lmax > 0) {
          visited[other] = true;
          next.push(other);
        }
      }
    }
    for (bool v : visited) {
      if (!v) {
        return false;
      }
    }
    return true;
  }
};

class NoCrossConstraint : public ExternalConstraint {
  const vector<Link>& links;
 public:
  NoCrossConstraint(const vector<Link>& links_) 
      : links(links_) {}

  virtual bool operator()(const vector<Variable>& variables) const {
    for (const auto& link : links) {
      for (int other : link.forbidden) {
        if (variables[link.id].lmin > 0 &&
            variables[other].lmin > 0) {
          return false;
        }
      }
    }
    return true;
  }
};

class ConstraintSolver {
  int recursion_nodes;
  vector<Variable> variables, solution;
  vector<Constraint> constraints;
  vector<const ExternalConstraint*> external;
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
    cout << "Recursion nodes: " << recursion_nodes << "\n";
  }

  bool recursion() {
    recursion_nodes++;
    if (finished()) {      
      solution = variables;
      return true;
    }
    int index = choose();
    vector<Variable> bkp = variables;
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
    int diff = numeric_limits<int>::max();
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

class HashiSolver {
 private:
  int width, height;
  const vector<string>& grid;
  vector<Node> nodes;
  vector<Link> links;
  ConstraintSolver solver;

 public:
  HashiSolver(int width_, int height_, const vector<string>& grid_)
      : width(width_), height(height_), grid(grid_) {
  }

  template<typename T>
  void for_all_digits(T callback) {
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        if (isdigit(grid[j][i])) {
          callback(j, i);
        }
      }
    }
  }

  void degeometrize() {
    vector<vector<int>> id(height, vector<int>(width, -1));

    for_all_digits([&](int j, int i) {
      id[j][i] = nodes.size();
      nodes.push_back(Node(i, j, grid[j][i] - '0', nodes.size()));
    });

    for_all_digits([&](int j, int i) {
      for (int ii = i + 1; ii < width; ii++) {
        if (isdigit(grid[j][ii])) {
          links.push_back(Link(id[j][i], id[j][ii], true, links.size()));
          break;
        }
      }
      for (int jj = j + 1; jj < height; jj++) {
        if (isdigit(grid[jj][i])) {
          links.push_back(Link(id[j][i], id[jj][i], false, links.size()));
          break;
        }
      }
    });

    for (auto& l1 : links) {
      if (l1.horizontal) {
        int y = nodes[l1.a].y;
        for (const auto& l2 : links) {
          int x = nodes[l2.a].x;
          if (y > nodes[l2.a].y && y < nodes[l2.b].y &&
              x > nodes[l1.a].x && x < nodes[l1.b].x) {
            l1.forbidden.insert(l2.id);
          }
        }        
      }
    }

    for (const auto& link : links) {
      nodes[link.a].links.insert(link.id);
      nodes[link.b].links.insert(link.id);
    }
  }

  void solve() {
    for (auto& link : links) {
      link.id = solver.create_variable(0, 2);
    }
    for (const auto& n : nodes) {
      auto cons = solver.create_constraint(n.size, n.size);
      for (auto link : n.links) {
        solver.add_variable(cons, links[link].id);
      }
    }
    if (nodes.size() > 2) {
      for (const auto& link : links) {
        if (nodes[link.a].size == nodes[link.b].size &&
            nodes[link.a].size <= 2) {
          int size = nodes[link.a].size;
          auto cons = solver.create_constraint(0, size - 1);
          solver.add_variable(cons, link.id);
        }
      }
    }
    NoCrossConstraint no_cross(links);
    solver.add_external_constraint(&no_cross);
    SingleGroupConstraint single_group(nodes, links);
    solver.add_external_constraint(&single_group);
    solver.solve();
    for (const auto& link : links) {
      auto var = solver.value(link.id);
      cout << "solution from node " << nodes[link.a].size
           << " to " << nodes[link.b].size << " is " 
           << "(" << var.lmin << " , " << var.lmax << ")\n";
    }
  }

  void print() {
    FILE *f = fopen("hashi.dot", "wt");
    fprintf(f, "graph {\n");
    for (const auto& n : nodes) {
      fprintf(f, "n%d_%d [label=%d\npos=\"%d,%d!\"]\n",
              n.id, n.size, n.size, n.x, height - n.y - 1);
    }
    for (const auto& link : links) {
      for (int i = 1; i <= solver.value(link.id).lmin; i++) {
        fprintf(f, "n%d_%d -- n%d_%d;\n", 
                link.a, nodes[link.a].size,
                link.b, nodes[link.b].size);
      }
    }
    fprintf(f, "}\n");
    fclose(f);
  }
};

int main() {
  int width, height;
  cin >> width;
  cin >> height;
  vector<string> grid(height);
  for (int i = 0; i < height; i++) {
    cin >> grid[i];
  }
  HashiSolver s(width, height, grid);
  s.degeometrize();
  s.solve();
  s.print();
  return 0;
}
