//
// Created by vwann on 10/23/2026.
//

#ifndef BENNETT_PARSER_H
#define BENNETT_PARSER_H


constexpr char reserved_c = 'f';

namespace Bennett {
    constexpr double interval = 0.02;                               //20 ms
    struct Data {
        //bad state reflects fail istream state or bad input
        //breath_end signal is sent from Bennett but is currently discarded
        enum data_type{bad, flow_pressure, breath_no, breath_end};

        Data() = default;                                       //default Data is fail type
        Data(data_type t, double f, double p) : t{t}, f{f}, p{p} { }
        explicit Data(int breath_number) : t{breath_no}, b_num{breath_number} { }

        data_type type() const {return t;}
        double flow() const {
            if (t!=flow_pressure) throw std::runtime_error("invalid type");
            return f;
        }
        double pressure() const {
            if (t!=flow_pressure) throw std::runtime_error("invalid type");
            return p;
        }
        int breath() const {
            if (t!=breath_no) throw std::runtime_error("invalid type");
            return b_num;
        }

    private:

        data_type t = bad;
        double f = 0;
        double p = 0;
        int b_num = 0;
    };

    inline bool get_flow_pressure(std::istream& is, Data& d) {
        char c = reserved_c;
        double f, p;
        is>>f>>c>>p;                //double , double
        if (c == ',') {
            if (is.good()) {        //p is good, flow-pressure is completed
                d = {Data::flow_pressure, f, p};
                return true;
            }
            if (!is.good()) {       //p is bad or eof, only flow is read
                if (is.eof()) {
                    // d = {Data::flow_pressure, f, NAN};  //if eof, p may be invalid, read as NAN
                    d = Data{};
                    return false;
                }
                d = Data{};         //if p is not double, bad input
                return false;
            }
        }
        d = d = Data{};
        return false;
    }
    inline bool look_ahead(std::istream& is) {          //returns true if BS/BE, returns false if failed read
        char c = reserved_c;
        while (!isdigit(c)) {
            is>>c;
            if (!is.good()) return false;
        }
        is.unget();
        return true;
    }
    inline bool get_breath_number(std::istream& is, Data& d) {
        if (look_ahead(is)) {
            int i;
            char c;
            is>>i>>c;
            if (is.good()) {
                d = Data{i};
                return true;
            }
        }
        d = Data{};                         //wrong format or reads until invalid is state
        return false;
    }
};

inline std::istream& operator>>(std::istream& is, Bennett::Data& d) {
    char c = reserved_c;
    is>>c;
    while (isalpha(c)&&is.good()) {
        if (c == 'B') {                    //BS, S:11810,
            is.get(c);
            if (c == 'S') {
                Bennett::get_breath_number(is, d);
                return is;
            }
            if (c == 'E') {             //BE = breath end, reads next breath
                is>>c;                  //next breath starts with BS, S::11811
            }
            else {
                d = Bennett::Data{};    //not BS or BE
                return is;
            }
        }
        else {                          //not begin with B
            d = Bennett::Data{}; return is;
        }
    }
    is.unget();
    Bennett::get_flow_pressure(is, d);      //returns default (bad) data if not digit
    return is;
}

inline std::ostream& operator<<(std::ostream& os, const Bennett::Data& data) {
    switch (data.type()) {
        case Bennett::Data::flow_pressure: {
            os<<data.flow()<<", "<<data.pressure()<<'\n';
            return os;
        }
        case Bennett::Data::breath_no: {
            os<<"BS, S:"<<data.breath()<<",\n";
            return os;
        }
        default:
            return os;      //does not pass bad data
    }
}

#endif //BENNETT_PARSER_H