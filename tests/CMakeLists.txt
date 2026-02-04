# Dans tests/CMakeLists.txt
project(Tests)

# On garde le test précédent s'il existe
# ...

# Ajouter le test des KPI
add_executable(test_kpi test_kpi.cpp ../src/core/simulation.cpp ../src/core/patient.cpp)
target_include_directories(test_kpi PRIVATE ../include)

# Test Comparatif Algorithmes
add_executable(test_algos test_algos.cpp ../src/core/simulation.cpp ../src/core/patient.cpp)
target_include_directories(test_algos PRIVATE ../include)

# Ajouter le test à la suite CTest
add_test(NAME TestKPI COMMAND test_kpi)
add_test(NAME TestAlgos COMMAND test_algos)