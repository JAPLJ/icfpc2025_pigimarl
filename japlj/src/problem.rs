pub const LABELS: usize = 4;
pub const DEGREE: usize = 6;

#[derive(Debug, Clone, Copy, PartialEq, Eq, strum::Display, strum::EnumString)]
#[strum(serialize_all = "lowercase")]
pub enum ProblemName {
    Probatio,
    Primus,
    Secundus,
    Tertius,
    Quartus,
    Quintus,
}

impl ProblemName {
    pub fn nodes(&self) -> usize {
        match self {
            ProblemName::Probatio => 3,
            ProblemName::Primus => 6,
            ProblemName::Secundus => 12,
            ProblemName::Tertius => 18,
            ProblemName::Quartus => 24,
            ProblemName::Quintus => 30,
        }
    }
}
