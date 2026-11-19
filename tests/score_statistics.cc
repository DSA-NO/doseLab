#include "ScoreStatistics.hh"

#include <cmath>
#include <stdexcept>

namespace {
void Check(bool condition)
{
  if (!condition) throw std::runtime_error("score statistics regression");
}
bool Close(double a, double b) { return std::abs(a - b) < 1.e-12; }
}

int main()
{
  ScoreStatistics left, right, empty;
  Check(std::isnan(left.StdDev(0)));
  left.Add(0., 0., 0.);
  Check(std::isnan(left.StdDev(0)));
  left.Add(1., 2., 4.);
  right.Add(3., 6., 12.);
  left.Merge(empty);
  empty.Merge(right);
  left.Merge(empty);
  Check(left.Count() == 3);
  Check(Close(left.Mean(0), 4. / 3.));
  Check(Close(left.M2(0), 14. / 3.));
  Check(Close(left.StdDev(0), std::sqrt(7. / 3.)));
  Check(Close(left.Mean(1), 8. / 3.));
  Check(Close(left.M2(2), 16. * 14. / 3.));
  left.Reset();
  Check(left.Count() == 0 && std::isnan(left.StdDev(0)));
}
