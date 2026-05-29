#ifndef SZILARD_ENGINE_H
#define SZILARD_ENGINE_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "../core/usf_constants.h"

class SzilardEngine : public Resource {
    GDCLASS(SzilardEngine, Resource);

protected:
    static void _bind_methods();

public:
    double negentropy = 0.0;
    double waste_heat = 0.0;
    double temperature = 300.0;
    double cold_bath_temp = 2.7;
    int bits_processed = 0;
    double efficiency = 0.0;

    Ref<USFConstants> constants;

    SzilardEngine();

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    void initialize(double initial_negentropy, double p_temperature);
    double extract_work(double bits);
    void inject_negentropy(double bits);
    double radiate_heat();
    bool is_alive() const;
    double landauer_cost(double bits) const;
    double metabolic_rate() const;
    double max_work_possible() const;
    void step(double dt);

    double get_negentropy() const { return negentropy; }
    double get_waste_heat() const { return waste_heat; }
    double get_temperature() const { return temperature; }
    double get_efficiency() const { return efficiency; }
    int get_bits_processed() const { return bits_processed; }
};

#endif // SZILARD_ENGINE_H
