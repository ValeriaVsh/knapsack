#include <iostream>
#include <vector>
#include <random>
#include <CbcModel.hpp>
#include <OsiClpSolverInterface.hpp>
#include <CglCutGenerator.hpp>
#include <OsiRowCut.hpp>

using namespace std;

class OddCutsGenerator : public CglCutGenerator
{
public:
    OddCutsGenerator() {}
    virtual ~OddCutsGenerator() {}

    virtual void generateCuts(const OsiSolverInterface& si, OsiCuts& cs,
        const CglTreeInfo info = CglTreeInfo()) override
    {
        if (info.inTree && info.pass > 0) return;

        int n = si.getNumCols();
        const double* solution = si.getColSolution();
        vector<int> items_inside_indexes;

        // Calculate the sum of variables in the current solution
        int sum = 0.0;
        for (int i = 0; i < n; ++i) {
            //sum += solution[i];
            
            if (solution[i] > 0.99) {
                items_inside_indexes.push_back(i);
                sum += 1;

            }
                
        }

        // Check if the sum is even (integer or fractional)
        if (sum % 2 == 0 && !items_inside_indexes.empty()) {
            // Create a cut to enforce an odd sum
            OsiRowCut cut;
            CoinPackedVector cutVector;
            for (int i = 0; i < n; ++i) {
                cutVector.insert(i, 1.0);
            }

            cut.setRow(cutVector);
            cut.setLb(1); // At least one item
            cut.setUb(1.0 * (2 * floor(items_inside_indexes.size() / 2)) - 1); // Force odd

            cs.insert(cut);

            CoinPackedVector compCut;
            for (int i = 0; i < n; ++i) {
                compCut.insert(i, -1.0);
            }

            OsiRowCut compRowCut;
            compRowCut.setRow(compCut);
            compRowCut.setLb(-(2 * ceil(items_inside_indexes.size() / 2) + 1));
            compRowCut.setUb(-1);
            cs.insert(compRowCut);
        }


    }
    virtual CglCutGenerator* clone() const override {
        return new OddCutsGenerator(*this);
    }

};

// Function to generate random data for the knapsack problem
void generate_data(int num_items, vector<int>& weights, vector<int>& values, int max_weight, int max_value) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> weight_dist(1, max_weight);
    uniform_int_distribution<> value_dist(1, max_value);

    for (int i = 0; i < num_items; ++i) {
        weights.push_back(weight_dist(gen));
        values.push_back(value_dist(gen));
    }
}

// Function to solve the knapsack problem
void solve_knapsack(const vector<int>& weights, const vector<int>& values, int capacity) 
{
    int num_items = weights.size();

    double* values_arr = new double[num_items];
    double* weights_arr = new double[num_items];
   

    for (int i = 0; i < num_items; i++)
    {
        values_arr[i] = static_cast<double>(values[i]);
        weights_arr[i] = static_cast<double>(weights[i]);
    }
   
   
    
    int* columns_indexes = new int[num_items];
    double* odd_constr_coefs = new double[num_items];
   

    for (int i = 0; i < (num_items); i++)
    {
        columns_indexes[i] = i;
        odd_constr_coefs[i] = 1.0;
        
    }
    
    
    // Create the solver
    OsiClpSolverInterface solver;
    solver.setObjSense(-1.0); // Maximize    

    for (int i = 0; i < num_items; i++)
    {
        
            solver.addCol(0, nullptr, nullptr, 0.0, 1.0, values_arr[i]); // Binary variable
            solver.setObjCoeff(i, values_arr[i]);       
       
    }
    

 
    solver.addRow(num_items, columns_indexes, weights_arr, 0.0, capacity); //ограничение на вес

    solver.setInteger(columns_indexes, num_items);// делает все переменые целочисленными (0 или 1)
    

    CbcModel model(solver);
    CglCutGenerator* cuts_gen = new OddCutsGenerator;
    model.addCutGenerator(cuts_gen,1, "Odd number of items", true, true);
    
    model.branchAndBound();

    int num_cols = solver.getNumCols();
    const double* solution = model.solver()->getColSolution();


    for (int iColumn = 0; iColumn < num_cols; iColumn++)
    {
        double value = solution[iColumn];
        cout << iColumn << " has value " << value << " price: " << values_arr[iColumn] << " weight: " << weights_arr[iColumn] << '\n';

    }

    cout << endl;
   

    int count_items = 0;
    double knapsack_weight = 0;
    double knapsack_price = 0;

    for (int iColumn = 0; iColumn < num_cols; iColumn++) 
    {
        double value = solution[iColumn];
        if (fabs(value) > 0.0)
        {
            knapsack_weight += weights[iColumn];
            knapsack_price += values[iColumn];
            count_items++;
        }

    }
    cout << "items in knapsack: " << count_items<<"\nknapsack weight: "<<knapsack_weight<<" <= "<<capacity
                                                <<"\nknapsack price: " <<knapsack_price<< endl;
    

    

   
    delete[] values_arr;
    delete[] weights_arr;
    delete[] columns_indexes;
    delete[] odd_constr_coefs;
    delete[] cuts_gen;
    

}

int main() {
    // Parameters
    int num_items, max_weight, max_value, capacity;
    do {
        cout << "Enter number of items from 200 to 1000: ";
        cin >> num_items;

    } while (num_items > 1000 || num_items < 200);
    
    
    cout << "\nEnter maximum weight of an item: ";
    cin >> max_weight;
    cout << "\nEnter maximum price of an item: ";
    cin >> max_value;
    cout << "\nEnter knapsack capacity: ";
    cin >> capacity;
    cout << endl;

    // Generate random weights and values
    vector<int> weights, values;
    generate_data(num_items, weights, values, max_weight, max_value);

    // Solve the knapsack problem
    solve_knapsack(weights, values, capacity);

   
    system("PAUSE");
    return 0;
}