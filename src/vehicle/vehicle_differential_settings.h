#pragma once

/*public class DifferentialData : ScriptableObject
{

	[Header("Setup")]
	public DiffType type = DiffType.Open;
	public float finalDriveRatio = 3.46f;

	[Header("LSD (clutch/viscous style)")]
	[Tooltip("Always-on coupling torque (Nm), even with zero speed difference.")]
	public float lsdPreloadNm = 80f;
	[Tooltip("Extra coupling per rad/s of side-to-side speed difference when accelerating.")]
	public float lsdLockCoefAccel = 120f;   // Nm per (rad/s)
	[Tooltip("Extra coupling per rad/s when engine braking.")]
	public float lsdLockCoefDecel = 60f;    // Nm per (rad/s)
	[Tooltip("Cap on the LSD's coupling torque (Nm).")]
	public float lsdMaxCouplingNm = 1500f;
	[Tooltip("Torque Bias Ratio cap: T_high ? T_low × TBR (1=open, 2–3 typical, 4–5 aggressive).")]
	[Range(1f, 6f)] public float lsdTorqueBiasRatio = 2.5f;

	[Header("Locked (spool)")]
	[Tooltip("Internal torque per rad/s of side-to-side speed difference.")]
	public float lockStiffness = 5000f;     // Nm per (rad/s)
	[Tooltip("Maximum coupling torque the spool applies.")]
	public float lockMaxCouplingNm = 5000f;
}*/

enum class DifferentialType {
	OPEN,
	LOCKED,
	LSD
};

class LNDifferentialSettings {
public:
	float lockStiffness = 5000.0f; // Nm per (rad/s)
	float lockMaxCouplingNm = 5000.0f;
	DifferentialType diff_type = DifferentialType::LOCKED;
};
