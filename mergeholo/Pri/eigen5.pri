# eigen5.pri - for qmake projects on Windows
# Eigen 5.0.0 installed at D:/ljc/eigen-5.0.0-install

EIGEN_ROOT = D:/ljc/eigen-5.0.0-install

INCLUDEPATH += $$EIGEN_ROOT/include/eigen3

# Eigen is header-only, no LIBS needed
