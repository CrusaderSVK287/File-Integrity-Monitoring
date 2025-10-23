#include <pybind11/embed.h>
#include <iostream>
#include <string>

namespace py = pybind11;

int main() {
    py::scoped_interpreter guard{};

    try {
        py::module mymodule = py::module::import("mymodule");

        // init
        py::dict init_params;
        // py::dict automatically converts strings/numbers for some fucking reason
        // spent like an hour on this crash
        init_params["param1"] = "value1";  

        py::dict init_result = mymodule.attr("init")(init_params).cast<py::dict>();
        std::string init_status = init_result["status"].cast<std::string>();
        std::cout << "Init returned: " << init_status << "\n";

        // run
        py::dict run_params;
        run_params["name"] = "Alice";
        run_params["count"] = 5;

        py::dict run_result = mymodule.attr("run")(run_params).cast<py::dict>();
        std::string run_message = run_result["message"].cast<std::string>();
        std::cout << "Run returned: " << run_message << "\n";

    } catch (py::error_already_set &e) {
        std::cerr << "Python error: " << e.what() << "\n";
    }

    return 0;
}

