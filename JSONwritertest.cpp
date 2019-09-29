#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace ns {
    struct Address {
        char HouseNo[25];
        char City[25];
        char PinCode[25];
    };

    struct Employee {
        int Id;
        char Name[25];
        float Salary;
        struct Address Add;
    };

    void to_json(json &j, const Employee &p) {
        j = //json
                {
                        {"Id",      p.Id},
                        {"Name",    p.Name},
                        {"Salary",  p.Salary},
                        {"Address", {
                                            {"HouseNo", p.Add.HouseNo},
                                            {"City", p.Add.City},
                                            {"PinCode", p.Add.PinCode}
                                    }

                        }
                };
    }
}

int main()
{
    // create a JSON array
    json j1 = {"one", "two", 3, 4.5, false};

    // create a copy
    json j2(j1);

    // serialize the JSON array
    std::cout << j1 << " = " << j2 << '\n';
    std::cout << std::boolalpha << (j1 == j2) << '\n';

    ns::Employee E;

               E.Id=3;
				std::string s;
				s= "testname................";
              strcpy(E.Name,s.c_str());

              E.Salary= 99.87;
				s="25......................";
               strcpy(E.Add.HouseNo,s.c_str());
				s="gotham.................";
              strcpy(E.Add.City,s.c_str());
				s="12345...................";
              strcpy(E.Add.PinCode,s.c_str());
              json j3;
              j3=E;
              json j4;
   j4={};


std::cout << j3.dump(2) << std::endl;

    // create a JSON object
    json j =
            {
                    {"pi", 3.141},
                    {"happy", true},
                    {"name", "Niels"},
                    {"nothing", nullptr},
                    {
                     "answer", {
                                   {"everything", 42}
                           }
                    },
                    {"list", {1, 0, 2}},
                    {
                     "object", {
                                   {"currency", "USD"},
                                 {"value", 42.99}
                           }
                    }
            };

    // add new values
    std::string test="nieuwer";
    j["new"]["key"]["value"] = {"another", "list"};
    j[test]["key"]["value"] = {"another", "list"};

    // count elements
    auto s1 = j.size();
    j["size"] = s1;

    // pretty print with indent of 4 spaces
    std::cout << std::setw(4) << j << '\n';
}
