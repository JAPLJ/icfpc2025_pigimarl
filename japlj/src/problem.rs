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
    Aleph,
    Beth,
    Gimel,
    Daleth,
    He,
    Vau,
    Zain,
    Hhet,
    Teth,
    Iod,
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
            ProblemName::Aleph => 12,
            ProblemName::Beth => 24,
            ProblemName::Gimel => 36,
            ProblemName::Daleth => 48,
            ProblemName::He => 60,
            ProblemName::Vau => 18,
            ProblemName::Zain => 36,
            ProblemName::Hhet => 54,
            ProblemName::Teth => 72,
            ProblemName::Iod => 90,
        }
    }
}
