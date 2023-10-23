


void meanVarCore(double *val, double *P, int size, double &mean, double &var)
{
  mean = var = 0; 
  // for (int i = 0; i < size + 100000; ++i) // Has a bug in this.
    for (int i = 0; i < size; ++i)
  {
    double tmp = val[i] * P[i];
    mean += tmp;
    var += tmp * val[i];
  }
  // std::cout << "yo\n";
  mean /= size;
  var = var / size - mean * mean;
}






















