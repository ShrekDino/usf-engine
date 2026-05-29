#include "thermo/szilard_engine.h"
#include "core/math/math_funcs.h"
#include <cmath>

SzilardEngine::SzilardEngine() {
    constants.instantiate();
    efficiency = 1.0 - cold_bath_temp / temperature;
    if (efficiency < 0.0) {
        efficiency = 0.0;
    }
}

void SzilardEngine::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
}

void SzilardEngine::initialize(double initial_negentropy, double p_temperature) {
    negentropy = initial_negentropy;
    temperature = p_temperature;
    waste_heat = 0.0;
    bits_processed = 0;
    efficiency = 1.0 - cold_bath_temp / temperature;
    if (efficiency < 0.0) {
        efficiency = 0.0;
    }
}

double SzilardEngine::extract_work(double bits) {
    if (bits > negentropy) {
        return 0.0;
    }
    double k_b = constants->k_b;
    double work = k_b * temperature * M_LN2 * bits * efficiency;
    negentropy -= bits;
    waste_heat += work * (1.0 - efficiency);
    bits_processed += (int)bits;
    return work;
}

void SzilardEngine::inject_negentropy(double bits) {
    negentropy += bits;
}

double SzilardEngine::radiate_heat() {
    double radiated = waste_heat * 0.5;
    waste_heat -= radiated;
    temperature -= radiated * 1e-10;
    if (temperature < 1e-20) {
        temperature = 1e-20;
    }
    return radiated;
}

bool SzilardEngine::is_alive() const {
    return negentropy > 0.0 && temperature > 0.0;
}

double SzilardEngine::landauer_cost(double bits) const {
    return constants->k_b * temperature * M_LN2 * bits;
}

double SzilardEngine::metabolic_rate() const {
    if (efficiency <= 0.0) {
        return 1e300;
    }
    return temperature * (1.0 - efficiency) / efficiency;
}

double SzilardEngine::max_work_possible() const {
    return negentropy * constants->k_b * temperature * M_LN2 * efficiency;
}

void SzilardEngine::step(double dt) {
    double base_heat = metabolic_rate() * dt;
    waste_heat += base_heat;
    negentropy -= base_heat * 0.01 / (constants->k_b * temperature);
    temperature += base_heat * 1e-12;
    radiate_heat();
}

void SzilardEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &SzilardEngine::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &SzilardEngine::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("initialize", "initial_negentropy", "temperature"), &SzilardEngine::initialize);
    ClassDB::bind_method(D_METHOD("extract_work", "bits"), &SzilardEngine::extract_work);
    ClassDB::bind_method(D_METHOD("inject_negentropy", "bits"), &SzilardEngine::inject_negentropy);
    ClassDB::bind_method(D_METHOD("radiate_heat"), &SzilardEngine::radiate_heat);
    ClassDB::bind_method(D_METHOD("is_alive"), &SzilardEngine::is_alive);
    ClassDB::bind_method(D_METHOD("landauer_cost", "bits"), &SzilardEngine::landauer_cost);
    ClassDB::bind_method(D_METHOD("metabolic_rate"), &SzilardEngine::metabolic_rate);
    ClassDB::bind_method(D_METHOD("max_work_possible"), &SzilardEngine::max_work_possible);
    ClassDB::bind_method(D_METHOD("step", "dt"), &SzilardEngine::step);

    ClassDB::bind_method(D_METHOD("get_negentropy"), &SzilardEngine::get_negentropy);
    ClassDB::bind_method(D_METHOD("get_waste_heat"), &SzilardEngine::get_waste_heat);
    ClassDB::bind_method(D_METHOD("get_temperature"), &SzilardEngine::get_temperature);
    ClassDB::bind_method(D_METHOD("get_efficiency"), &SzilardEngine::get_efficiency);
    ClassDB::bind_method(D_METHOD("get_bits_processed"), &SzilardEngine::get_bits_processed);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "negentropy", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_negentropy");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "waste_heat", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_waste_heat");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temperature", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_temperature");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "efficiency", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_efficiency");
}
