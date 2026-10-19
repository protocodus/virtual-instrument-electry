"""UNEXECUTED controller/lifecycle prototype. No field, solver or audio code.

An immutable recipe contains already-owned input bits; an instance contains
only event-relative support/controller state. The caller owns all q/v, active
contact storage and the uninterrupted E+U-W+D ledger. No numerical imports.
"""
if not __debug__:
    raise SystemExit('Optimized Python forbidden')
from collections import deque
from dataclasses import dataclass
from fractions import Fraction
import math

RATE = 96000
STIFFNESS_N_PER_M = 6000.0  # inherited numerical fixture, not measured hardware


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def ticks(value):
    require(isinstance(value, (int, Fraction)) and not isinstance(value, bool),
            'Use exact local integer/Fraction96k ticks, never absolute float time')
    return Fraction(value)


@dataclass(frozen=True)
class ContactRecipe:
    recipe_id: str
    geometry_key: str  # immutable basis/mass/Omega/alpha/B ownership, not MIDI
    press_frames96: int
    pulse: tuple[float, ...]
    release_coefficient: float
    D_m: float
    source_provenance: str

    def __post_init__(self):
        require(type(self.press_frames96) is int and self.press_frames96 > 0, 'Owned positive Contact length')
        require(isinstance(self.pulse, tuple) and len(self.pulse) > 0, 'Owned immutable full pulse')
        require(all(type(x) is float and math.isfinite(x) for x in self.pulse), 'Literal finite binary64 pulse')
        require(self.pulse[0] == 0.0, 'Same zero first withdrawal knot as frozen source')
        require(type(self.release_coefficient) is float and 0 <= self.release_coefficient < 1,
                'Owned stable one-pole, no new coefficient selection')
        require(type(self.D_m) is float and math.isfinite(self.D_m) and self.D_m >= 0, 'Owned strokeD bits')


@dataclass(frozen=True)
class Arrival:
    event_id: str
    contact_boundary_tick96: int
    first_contact_rendered_engine_clock: int | None  # None means unowned, not an inferred offset
    start_order: int

    def __post_init__(self):
        require(all(type(x) is int and x >= 0 for x in (self.contact_boundary_tick96,
                self.start_order)), 'Owned nonnegative integer boundary/order metadata')
        require(self.first_contact_rendered_engine_clock is None
                or (type(self.first_contact_rendered_engine_clock) is int and self.first_contact_rendered_engine_clock >= 0),
                'Actual rendered clock or explicitly unowned; never manufacture +/-1')

    @property
    def key(self):
        return self.contact_boundary_tick96, self.start_order


@dataclass(frozen=True)
class PortSnapshot:
    boundary_tick96: int
    geometry_key: str
    y_m: float
    velocity_m_per_s: float
    qv_identity: str       # caller evidence token; this module never reads q/v
    lifetime_ledger_identity: str

    def __post_init__(self):
        require(type(self.boundary_tick96) is int, 'Actual integer endpoint boundary')
        require(math.isfinite(self.y_m) and math.isfinite(self.velocity_m_per_s), 'Finite incoming gather')


@dataclass(frozen=True)
class SupportValue:
    local_tick96: Fraction
    side: str
    segment: str
    z_m: float | None
    velocity_m_per_s: float | None
    acceleration_m_per_s2: float | None


@dataclass(frozen=True)
class WithdrawalKnot:
    index: int
    filtered_mass: float
    cumulative_mass: float
    z_m: float


class WithdrawalCursor:
    """Bounded three-knot cache; source-only preview of a declared trajectory.

    The extra planned knot supplies the next interval slope, not future string
    audio. General backward ODE trial queries are deliberately unsupported.
    For beta=float(1-a), the exact-real recurrence has total mass
    beta/(1-a_real)*sum(storedpulse), not an assumed exact1. Binary64 cumulative
    mass can additionally stall. Neither this cursor nor its limit guarantees
    finite contact retirement for arbitrary incoming phase/weakD (including0).
    """
    def __init__(self, recipe, y0):
        self.recipe, self.y0 = recipe, y0
        self.stored_state = 0.0
        self.cumulative_mass = 0.0
        self.last_generated = -1
        self.cache = deque(maxlen=3)
        self.b0 = 1 - recipe.release_coefficient

    def ensure(self, last):
        require(type(last) is int and last >= 0, 'Nonnegative integer withdrawal knot')
        while self.last_generated < last:
            j = self.last_generated + 1
            p = self.recipe.pulse[j] if j < len(self.recipe.pulse) else 0.0
            # Explicit DF-II-transposed candidate order, as documented by
            # SciPy lfilter; compiled reference FMA/bitidentity is UNTESTED.
            output = self.b0 * p + self.stored_state
            self.stored_state = self.recipe.release_coefficient * output
            self.cumulative_mass = self.cumulative_mass + output
            z = self.y0 + self.recipe.D_m - 2 * self.recipe.D_m * self.cumulative_mass
            require(all(math.isfinite(x) for x in (output, self.stored_state, self.cumulative_mass, z)),
                    'Nonfinite controller: do not clip/renormalize/restart')
            self.cache.append(WithdrawalKnot(j, output, self.cumulative_mass, z))
            self.last_generated = j

    def knot(self, j):
        for value in self.cache:
            if value.index == j:
                return value
        raise RuntimeError('Backward query outside three-knot cache; no hidden reset/replay')


class ContactInstance:
    def __init__(self, recipe, arrival, snapshot):
        require(recipe.geometry_key == snapshot.geometry_key, 'Fixed geometry ownership')
        require(snapshot.boundary_tick96 == arrival.contact_boundary_tick96, 'Actual arrival boundary')
        require(all(math.isfinite(x) for x in (snapshot.y_m, snapshot.velocity_m_per_s)), 'Finite incoming gather')
        self.recipe, self.arrival, self.attachment = recipe, arrival, snapshot
        self.y0, self.v0 = snapshot.y_m, snapshot.velocity_m_per_s
        self.withdrawal = WithdrawalCursor(recipe, self.y0)
        self.active = True  # independent of legacy Voice/ExcitationPhase
        self.contact_U_J = 0.0  # unstrained attach; q/v and old ledger untouched
        self.last_endpoint_tick96 = arrival.contact_boundary_tick96
        self.last_qv_identity = snapshot.qv_identity
        self.last_ledger_identity = snapshot.lifetime_ledger_identity

    def local_tick(self, absolute_tick96):
        # Integer subtraction precedes every float conversion, even at late
        # event origins. No universal engineclock+/-1 mapping is inferred.
        return ticks(absolute_tick96) - self.arrival.contact_boundary_tick96

    def value(self, absolute_tick96, side='right'):
        require(side in ('left', 'right'), 'Declared one-sided support')
        local = self.local_tick(absolute_tick96)
        if local < 0:
            return SupportValue(local, side, 'not_attached', None, None, None)
        press = self.recipe.press_frames96
        duration = press / RATE
        if local < press or (local == press and side == 'left'):
            # Same press polynomial and endpoint rules. Local time avoids
            # large absolute cancellation; this is NOT an all-origins bit claim.
            s = (float(local) / RATE) / duration
            if local == 0:
                s = 0.0
            if local == press:
                s = 1.0
            D = self.recipe.D_m
            z = self.y0 + duration * self.v0 * (s - 2*s*s + s*s*s) + D * (3*s*s - 2*s*s*s)
            zd = self.v0 * (1 - 4*s + 3*s*s) + D * (6*s - 6*s*s) / duration
            zdd = self.v0 * (-4 + 6*s) / duration + D * (6 - 12*s) / duration**2
            return SupportValue(local, side, 'press', z, zd, zdd)
        release = local - press
        j = release.numerator // release.denominator
        self.withdrawal.ensure(j + 1)
        current, following = self.withdrawal.knot(j), self.withdrawal.knot(j + 1)
        if release.denominator == 1:
            if side == 'left':
                previous = self.withdrawal.knot(j - 1)
                slope = (current.z_m - previous.z_m) * RATE
            else:
                slope = (following.z_m - current.z_m) * RATE
            return SupportValue(local, side, 'withdrawal_knot', current.z_m, slope, 0.0)
        slope = (following.z_m - current.z_m) * RATE
        z = current.z_m + (float(release - j) / RATE) * slope
        return SupportValue(local, side, 'withdrawal_interval', z, slope, 0.0)

    def observe_endpoint(self, snapshot, contact_U_J):
        """Bookkeeping from a later qualified mechanical owner, not a solve."""
        require(self.active and snapshot.geometry_key == self.recipe.geometry_key, 'Active fixed-geometry contact')
        require(snapshot.boundary_tick96 >= self.last_endpoint_tick96, 'Monotone endpoint bookkeeping')
        require(math.isfinite(contact_U_J) and contact_U_J >= 0, 'Actual finite nonnegative storedU')
        self.contact_U_J = contact_U_J
        self.last_endpoint_tick96 = snapshot.boundary_tick96
        self.last_qv_identity = snapshot.qv_identity
        self.last_ledger_identity = snapshot.lifetime_ledger_identity

    def try_retire(self, snapshot):
        require(self.active and snapshot.boundary_tick96 == self.last_endpoint_tick96,
                'Retirement needs current endpoint storage evidence')
        require(snapshot.geometry_key == self.recipe.geometry_key
                and snapshot.qv_identity == self.last_qv_identity
                and snapshot.lifetime_ledger_identity == self.last_ledger_identity,
                'Retirement storage and gather must belong to the same current owner snapshot')
        right = self.value(snapshot.boundary_tick96, 'right')
        eligible = (right.local_tick96 >= self.recipe.press_frames96
                    and right.z_m - snapshot.y_m <= 0
                    and right.velocity_m_per_s - snapshot.velocity_m_per_s <= 0)
        if not eligible:
            return {'status': 'not_eligible', 'physical_state_mutated': False}
        if self.contact_U_J != 0.0:
            return {'status': 'positive_U_retirement_forbidden', 'U_J': self.contact_U_J,
                    'physical_state_mutated': False}
        self.active = False
        return {'status': 'retired_zero_U', 'event_id': self.arrival.event_id,
                'boundary_tick96': snapshot.boundary_tick96, 'removed_contact_U_J': 0.0,
                'qv_identity_unchanged': snapshot.qv_identity,
                'lifetime_ledger_identity_unchanged': snapshot.lifetime_ledger_identity,
                'no_qv_reset_or_work_D_restart': True}


class SameGeometryContactService:
    """One mechanical pick port; no queue, overlap rule or fret transport."""
    def __init__(self, geometry_key):
        self.geometry_key = geometry_key
        self.current = None
        self.last_arrival_key = None

    def request_attach(self, recipe, arrival, snapshot):
        require(recipe.geometry_key == snapshot.geometry_key == self.geometry_key, 'No geometry change in this service')
        require(type(arrival.contact_boundary_tick96) is int and type(arrival.start_order) is int,
                'Owned integer arrival/order')
        require(snapshot.boundary_tick96 == arrival.contact_boundary_tick96, 'Gather belongs to actual arrival')
        require(self.last_arrival_key is None or arrival.key > self.last_arrival_key,
                'Strict actual-arrival/startOrder ordering; no duplicate retry')
        self.last_arrival_key = arrival.key
        if self.current is not None and self.current.active:
            if self.current.last_endpoint_tick96 != arrival.contact_boundary_tick96:
                status = 'busy_active_port_stale_endpoint_UNRESOLVED'
            elif self.current.contact_U_J > 0:
                status = 'busy_positive_U_UNRESOLVED'
            else:
                status = 'busy_zero_U_not_retired_UNRESOLVED'
            return {'status': status, 'new_event_id': arrival.event_id,
                    'old_event_id': self.current.arrival.event_id,
                    'old_U_J': self.current.contact_U_J,
                    'old_instance_unchanged': True, 'incoming_qv_ledger_unchanged': True,
                    'no_implicit_reset_queue_delay_or_old_contact_delete': True}
        self.current = ContactInstance(recipe, arrival, snapshot)
        return {'status': 'attached_unstrained', 'event_id': arrival.event_id,
                'contact_U_J': 0.0, 'qv_identity_unchanged': snapshot.qv_identity,
                'lifetime_ledger_identity_unchanged': snapshot.lifetime_ledger_identity,
                'first_contact_rendered_clock_owned': arrival.first_contact_rendered_engine_clock is not None,
                'no_mechanical_state_created_or_initialized_here': True}


if __name__ == '__main__':
    raise SystemExit('Source-only DRAFT: no recipe/path/service evaluation authorized')
