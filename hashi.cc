#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <set>
#include <cstdio>
#include <limits>

using namespace std;

struct Node {
  int x, y;
  int size;
  int id;
  set<int> links;
};

struct Link {
  int a, b;
  bool horizontal;
  set<int> forbidden;
  int id;
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

class NoCrossConstraint : public ExternalConstraint {
  const vector<Link>& links;
 public:
  NoCrossConstraint(const vector<Link>& links_) 
      : links(links_) {}

  virtual bool operator()(const vector<Variable>& variables) const {
    cout << "no cross constraint checked\n";
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
    vector<Variable> bkp(variables.begin(), variables.end());
    auto var = variables[index];
    for (int i = var.lmin; i <= var.lmax; i++) {
      copy(bkp.begin(), bkp.end(), variables.begin());
      auto& var = variables[index];
      var.lmin = var.lmax = i;
      if (tight() && valid()) {
        cout << "is valid\n";
        if (recursion()) {
          return true;
        }
      }
    }
    copy(bkp.begin(), bkp.end(), variables.begin());
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
            cout << "changed var " << ivar << " from " << var.lmin 
                 << " to " << limit << "\n";
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
            cout << "changed var " << ivar << " from " << var.lmax
                 << " to " << limit << "\n";
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

  void degeometrize() {
    vector<vector<int>> id(height, vector<int>(width, -1));

    int current = 0;
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        if (isdigit(grid[j][i])) {
          Node n;
          n.x = i;
          n.y = j;
          n.size = grid[j][i] - '0';
          n.id = current;
          id[j][i] = current++;
          nodes.push_back(n);
        }
      }
    }

    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        if (isdigit(grid[j][i])) {
          for (int ii = i + 1; ii < width; ii++) {
            if (isdigit(grid[j][ii])) {
              Link link;
              link.a = id[j][i];
              link.b = id[j][ii];
              link.horizontal = true;
              link.id = links.size();
              links.push_back(link);
              break;
            }
          }
          for (int jj = j + 1; jj < height; jj++) {
            if (isdigit(grid[jj][i])) {
              Link link;
              link.a = id[j][i];
              link.b = id[jj][i];
              link.horizontal = false;
              link.id = links.size();
              links.push_back(link);
              break;
            }
          }
        } 
      }
    } 

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
    cout << "start solver\n";
    for (auto& link : links) {
      link.id = solver.create_variable(0, 2);
    }
    for (const auto& n : nodes) {
      auto cons = solver.create_constraint(n.size, n.size);
      for (auto link : n.links) {
        solver.add_variable(cons, links[link].id);
      }
    }
    NoCrossConstraint no_cross(links);
    solver.add_external_constraint(&no_cross);
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
    for (int i = 0; i < int(nodes.size()); i++) {
      auto n = nodes[i];
      fprintf(f, "n%d_%d [ label=%d\npos=\"%d,%d!\"]\n",
              i, n.size, n.size, n.x, height - n.y - 1);
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
