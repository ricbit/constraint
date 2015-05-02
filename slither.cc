#include <iostream>
#include <vector>
#include <cctype>
#include <cstdio>
#include <queue>
#include "constraint.h"

using namespace std;

class PointConstraint : public ExternalConstraint {
  int y, x, width, height;
  const vector<vector<int>>& vert;
  const vector<vector<int>>& horiz;
 public:
  PointConstraint(int y_, int x_, int width_, int height_,
    const vector<vector<int>>& vert_, const vector<vector<int>>& horiz_) 
      : y(y_), x(x_), width(width_), height(height_), 
        vert(vert_), horiz(horiz_) {}

  virtual bool operator()(const vector<Variable>& variables) const {
    int minsum = 0;
    int detsum = 0;
    int detvalue = 0;
    int alllinks = 0;
    if (x < width) {
      minsum += variables[horiz[y][x]].lmin;
      alllinks++;
      if (variables[horiz[y][x]].lmin == variables[horiz[y][x]].lmax) {
        detsum++;
        detvalue += variables[horiz[y][x]].lmin;
      }
    }
    if (x > 0) {
      minsum += variables[horiz[y][x - 1]].lmin;
      alllinks++;
      if (variables[horiz[y][x - 1]].lmin == variables[horiz[y][x - 1]].lmax) {
        detsum++;
        detvalue += variables[horiz[y][x - 1]].lmin;
      }
    }
    if (y < height) {
      minsum += variables[vert[y][x]].lmin;
      alllinks++;
      if (variables[vert[y][x]].lmin == variables[vert[y][x]].lmax) {
        detsum++;
        detvalue += variables[vert[y][x]].lmin;
      }
    }
    if (y > 0) {
      minsum += variables[vert[y - 1][x]].lmin;
      alllinks++;
      if (variables[vert[y - 1][x]].lmin == variables[vert[y - 1][x]].lmax) {
        detsum++;
        detvalue += variables[vert[y - 1][x]].lmin;
      }
    }
    if (minsum > 2) {
      cout << "failed " << y << " " << x << " minsum " << minsum << "\n";
      return false;
    }
    if (alllinks != detsum) {
      return true;
    }
    if (detvalue == 0 || detvalue == 2) {
      return true;
    }
    cout << "failed " << y << " " << x << " detvalue " << detvalue << "\n";
    return false;
  }
};

class SlitherLinkSolver {
  int width, height;
  const vector<string>& grid;
  ConstraintSolver solver;
  vector<vector<int>> vert, horiz;
 public:
  SlitherLinkSolver(int width_, int height_, const vector<string>& grid_)
      : width(width_), height(height_), grid(grid_) {}

  void solve() {
    vert.resize(height);
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width + 1; i++) {
        vert[j].push_back(solver.create_variable(0, 1));
      }
    }
    horiz.resize(height + 1);
    for (int j = 0; j < height + 1; j++) {
      for (int i = 0; i < width; i++) {
        horiz[j].push_back(solver.create_variable(0, 1));
      }
    }
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        if (isdigit(grid[j][i])) {
          int size = grid[j][i] - '0';
          auto cons = solver.create_constraint(size, size);
          solver.add_variable(cons, horiz[j][i]);
          solver.add_variable(cons, horiz[j + 1][i]);
          solver.add_variable(cons, vert[j][i]);
          solver.add_variable(cons, vert[j][i + 1]);
        }
      }
    }
    vector<PointConstraint*> external;
    for (int j = 0; j < height + 1; j++) {
      for (int i = 0; i < width + 1; i++) {
        PointConstraint *pc = new PointConstraint(
            j, i, width, height, vert, horiz);
        external.push_back(pc);
        solver.add_external_constraint(pc);
      }
    }
    solver.solve();
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width + 1; i++) {
        cout << "vert " << j << " " << i << " " 
             << solver.value(vert[j][i]).lmin << "\n";
      }
    }
    for (int j = 0; j < height + 1; j++) {
      for (int i = 0; i < width; i++) {
        cout << "horiz " << j << " " << i << " "
             << solver.value(horiz[j][i]).lmin << "\n";
      }
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
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width + 1; i++) {
        if (solver.value(vert[j][i]).lmin > 0) {
          fprintf(f, "n%d_%d -- n%d_%d;\n", j, i, j + 1, i); 
        }
      }
    }
    for (int j = 0; j < height + 1; j++) {
      for (int i = 0; i < width; i++) {
        if (solver.value(horiz[j][i]).lmin > 0) {
          fprintf(f, "n%d_%d -- n%d_%d;\n", j, i, j, i + 1); 
        }
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
  s.solve();
  s.print();
  return 0;
}
