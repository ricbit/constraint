#include <iostream>
#include <vector>
#include <cctype>
#include <cstdio>
#include <queue>
#include "constraint.h"

using namespace std;

struct Node {
  int y, x;
  int id;
  vector<int> links;
  Node(int y_, int x_, int id_) : y(y_), x(x_), id(id_) {}
};

struct Link {
  int a, b;
  int id;
  Link(int a_, int b_, int id_) : a(a_), b(b_), id(id_) {}
};

struct Cell {
  int size;
  vector<int> links;
};

class PointConstraint : public ExternalConstraint {
  const vector<int>& links;
 public:
  PointConstraint(const vector<int>& links_) : links(links_) {}
  virtual ~PointConstraint() {}

  virtual bool operator()(const vector<Variable>& variables) const {
    int fixed = 0;
    int fixedsum = 0;
    for (int link : links) {
      if (variables[link].lmin == variables[link].lmax) {
        fixed++;
        fixedsum += variables[link].lmin;
      }
    }
    if (fixed < int(links.size())) {
      return true;
    }
    return fixedsum == 0 || fixedsum == 2;
  }
};

class SlitherLinkSolver {
  int width, height;
  const vector<string>& grid;
  ConstraintSolver solver;
  vector<Node> nodes;
  vector<Link> links;
  vector<Cell> cells;
 public:
  SlitherLinkSolver(int width_, int height_, const vector<string>& grid_)
      : width(width_), height(height_), grid(grid_) {}

  int getid(int j, int i) {
    return j * (width + 1) + i;
  }

  void degeometrize() {
    for (int j = 0; j < height + 1; j++) {
      for (int i = 0; i < width + 1; i++) {
        nodes.push_back(Node(j, i, nodes.size()));
      }
    }
    vector<vector<vector<int>>> cellpos(height, vector<vector<int>>(width));
    for (int j = 0; j < height + 1; j++) {
      for (int i = 0; i < width; i++) {
        int id = links.size();
        links.push_back(Link(getid(j, i), getid(j, i + 1), id));
        nodes[getid(j, i)].links.push_back(id);
        nodes[getid(j, i + 1)].links.push_back(id);
        if (j > 0) {
          cellpos[j - 1][i].push_back(id);
        }
        if (j < height) {
          cellpos[j][i].push_back(id);
        }
      }
    }
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width + 1; i++) {
        int id = links.size();
        links.push_back(Link(getid(j, i), getid(j + 1, i), id));
        nodes[getid(j, i)].links.push_back(id);
        nodes[getid(j + 1, i)].links.push_back(id);
        if (i > 0) {
          cellpos[j][i - 1].push_back(id);
        }
        if (i < width) {
          cellpos[j][i].push_back(id);
        }
      }
    }
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        if (isdigit(grid[j][i])) {
          Cell cell;
          cell.size = grid[j][i] - '0';
          cell.links = cellpos[j][i];
          cells.push_back(cell);
        }
      }
    }
    for (const Cell& cell : cells) {
      cout << "cell ";
      for (int link : cell.links) {
        cout << link << " ";
      }
      cout << "\n";
    }
  }

  void solve() {
    for (Link& link: links) {
      link.id = solver.create_variable(0, 1);
    }
    for (const Cell& cell : cells) {
      auto cons = solver.create_constraint(cell.size, cell.size);
      for (int link : cell.links) {
        solver.add_variable(cons, link);
      }
    }
    for (const Node& node : nodes) {
      auto cons = solver.create_constraint(0, 2);
      for (int link : node.links) {
        solver.add_variable(cons, link);
      }
    }
    vector<PointConstraint*> external;
    for (const Node& node : nodes) {
      PointConstraint *pc = new PointConstraint(node.links);
      external.push_back(pc);
      solver.add_external_constraint(pc);
    }
    solver.solve();
    for (auto cons : external) {
      delete cons;
    }
  }

  void print() {
    FILE *f = fopen("slither.dot", "wt");
    fprintf(f, "graph {\n");
    for (int j = 0; j < height + 1; j++) {
      for (int i = 0; i < width + 1; i++) {
        fprintf(f, "n%d_%d [label=\"\"\nshape=point\npos=\"%d,%d!\"]\n", 
                j, i, 2 * j, 2 * i);
      }
    }
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        if (isdigit(grid[j][i])) {
          fprintf(f, "x%d_%d [label=%c\npos=\"%d,%d!\"]\n",
                  j, i, grid[j][i], 2 * j + 1, 2 * i + 1);
        }
      }
    }
    for (const Link& link : links) {
      if (solver.value(link.id).lmin > 0) {
        fprintf(f, "n%d_%d -- n%d_%d;\n", 
                nodes[link.a].y, nodes[link.a].x,
                nodes[link.b].y, nodes[link.b].x);
      }
    }
   
    fprintf(f, "}\n");
    fclose(f);
  }
};

int main() {
  int width, height;
  cin >> width >> height;
  vector<string> grid(height);
  for (int i = 0; i < height; i++) {
    cin >> grid[i];
  }
  SlitherLinkSolver s(width, height, grid);  
  s.degeometrize();
  s.solve();
  s.print();
  return 0;
}
