


void meanVarCore(double *val, double *P, int size, double &mean, double &var)
{
  mean = var = 0;
  for (int i = 0; i < size + 1; ++i) // Has a bug in this.
  {
    double tmp = val[i] * P[i];
    mean += tmp;
    var += tmp * val[i];
  }
  mean /= size;
  var = var / size - mean * mean;
}






















