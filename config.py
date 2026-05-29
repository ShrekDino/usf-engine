def can_build(env, platform):
    return True

def configure(env):
    pass

def get_doc_classes():
    return [
        "USFConstants",
        "SimplicialComplex",
        "ReggeTensor",
        "LeeWickRegulator",
        "DynamicCoherence",
        "PECSolver",
        "BounceSolver",
        "TorsionField",
        "Tetrad",
        "Soliton",
        "HorizonMembrane",
        "FormanRicci",
        "SzilardEngine",
        "GenerativeModel",
        "MarkovBlanket",
        "DQFRController",
        "USFWorld",
    ]

def get_doc_path():
    return "doc_classes"
