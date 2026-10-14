#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

#include "../engine/RelationalChamber.hpp"
#include "../engine/ChamberResponse.hpp"
#include "../engine/ChamberABReport.hpp"
#include "../engine/OpenModuleArchive.hpp"
#include "../engine/OrganismStemAuditor.hpp"

using namespace rasgo;

namespace {

void require(bool condition) {
    if (!condition) std::abort();
}

ChamberObservation body(double midi, double energy, double attack, double tension) {
    ChamberObservation result;
    result.manifested = energy > 0.001;
    result.midi = midi;
    result.energy = energy;
    result.attack = attack;
    result.tension = tension;
    return result;
}

void deterministic_and_provenant() {
    auto flute = body(72.0, 0.35, 0.4, 0.2);
    auto tuba = body(48.0, 0.42, 0.6, 0.7);
    auto voice = body(67.0, 0.22, 0.2, 0.4);
    auto archive = body(60.0, 0.18, 0.1, 0.3);
    archive.archivePage = 91;

    RelationalChamber first(1234u);
    RelationalChamber second(1234u);
    const auto a = first.evaluate(flute, tuba, voice, archive, 4);
    const auto b = second.evaluate(flute, tuba, voice, archive, 4);
    require(a.action == b.action);
    require(a.source == b.source);
    require(a.target == b.target);
    require(a.sourcePage == 91);
    require(a.consequence == b.consequence);
    require(a.manifestedBodies == 4);
    require(a.hasManifestationEvidence());
    require(a.isCausallyInfluential() == a.consequence);
    require(first.decisionCount() == 1);
    require(second.decisionCount() == 1);
    require(std::string(chamberActionName(a.action))
        == std::string(chamberActionName(b.action)));
}

void action_names_cover_contract() {
    require(std::string(chamberActionName(ChamberAction::None)) == "none");
    require(std::string(chamberActionName(ChamberAction::Cite)) == "cite");
    require(std::string(chamberActionName(ChamberAction::Resist)) == "resist");
    require(std::string(chamberActionName(ChamberAction::Deform)) == "deform");
    require(std::string(chamberActionName(ChamberAction::Scar)) == "scar");
}

void silence_does_not_invent_relation() {
    RelationalChamber chamber(7u);
    const auto silent = ChamberObservation{};
    const auto decision = chamber.evaluate(silent, silent, silent, silent, 1);
    require(decision.action == ChamberAction::None);
    require(!decision.consequence);
    require(decision.manifestedBodies == 0);
    require(chamber.relationCount() == 0);
    require(chamber.decisionCount() == 1);
    require(std::fabs(chamber.memory()) < 1.0e-12);
}

void memory_and_resistance_are_bar_rate() {
    RelationalChamber chamber(9u);
    const auto flute = body(72.0, 0.8, 0.8, 0.2);
    const auto tuba = body(48.0, 0.8, 0.8, 0.9);
    const auto voice = body(67.0, 0.7, 0.4, 0.8);
    const auto archive = ChamberObservation{};
    const auto decision = chamber.evaluate(flute, tuba, voice, archive, 2);
    require(decision.memory > 0.0);
    require(decision.action == ChamberAction::Resist
        || decision.action == ChamberAction::Deform);
    require(decision.consequence);
    require(decision.pressure >= 0.0 && decision.pressure <= 1.0);
    require(decision.memory >= 0.0 && decision.memory <= 1.0);
    require(decision.interval >= -24.0 && decision.interval <= 24.0);
    require(chamber.relationCount() == 1);
    require(chamber.decisionCount() == 1);
}

void completed_section_snapshot_reaches_chamber_after_reset() {
    OrganismStemAuditor auditor;
    auditor.prepare(48000.0, false, false, 0.6, 0.6);
    auditor.beginSection();
    for (int frame = 0; frame < 64; ++frame) {
        auditor.beginFrame();
        auditor.addPreReverb(OrganismStem::FmFlute, 0.20, 0.20);
        auditor.addPreReverb(OrganismStem::Tuba, 0.16, 0.16);
        auditor.addPreReverb(OrganismStem::Core, 0.12, 0.12);
        auditor.closePreReverb(0.48, 0.48);
    }
    const auto completed = auditor.sectionSnapshot();
    auditor.beginSection();
    require(auditor.sectionMetrics(OrganismStem::FmFlute).frames == 0);

    const auto observation = [&](OrganismStem stem, double midi) {
        const auto &metrics = completed[static_cast<std::size_t>(stem)];
        ChamberObservation result;
        result.manifested = metrics.audibleRatio > 0.004;
        result.midi = midi;
        result.energy = std::clamp(metrics.rms * 8.0, 0.0, 1.0);
        result.attack = std::clamp(metrics.peak * 4.0, 0.0, 1.0);
        result.tension = std::clamp(1.0 - metrics.audibleRatio, 0.0, 1.0);
        return result;
    };
    RelationalChamber chamber(101u);
    const auto decision = chamber.evaluate(
        observation(OrganismStem::FmFlute, 72.0),
        observation(OrganismStem::Tuba, 48.0),
        observation(OrganismStem::Core, 67.0),
        ChamberObservation{}, 4);
    require(decision.manifestedBodies == 3);
    require(decision.hasManifestationEvidence());
    require(decision.consequence);
}

void disabled_is_neutral() {
    RelationalChamber chamber(33u, false);
    const auto active = body(60.0, 1.0, 1.0, 1.0);
    const auto decision = chamber.evaluate(active, active, active, active, 8);
    require(decision.action == ChamberAction::None);
    require(!decision.consequence);
    require(chamber.memory() == 0.0);
    require(chamber.decisionCount() == 0);
}

void archive_read_preserves_provenance() {
    OpenModuleArchive archive;
    const RegionId source = 3;
    const RegionId reader = 7;
    const auto original = archive.publish(
        source, ModuleDomain::Memory, 0.42, -5.0, 2);
    const auto read = archive.readLatest(reader, ModuleDomain::Memory, 4);
    require(read.has_value());
    require(read->id == original);
    require(archive.count(ArchiveAction::Read) == 1);
    const auto reinterpretation = archive.reinterpret(
        reader, read->id, ModuleDomain::Memory, 0.61, -3.0, 5,
        ArchiveAction::Scar);
    const auto page = archive.page(reinterpretation);
    require(page.has_value());
    require(page->parent == original);
    require(page->source == source);
    require(page->reader == reader);
    require(page->action == ArchiveAction::Scar);
}

void chamber_reuses_memory_page_identity() {
    OpenModuleArchive archive;
    const auto page = archive.publish(
        2, ModuleDomain::Memory, 0.8, 0.25, 3);
    const auto read = archive.readLatest(9, ModuleDomain::Memory, 6, true);
    require(read.has_value());

    ChamberObservation flute = body(72.0, 0.4, 0.3, 0.2);
    ChamberObservation tuba = body(48.0, 0.5, 0.4, 0.7);
    ChamberObservation voice = body(67.0, 0.3, 0.2, 0.4);
    ChamberObservation memory = body(60.0, read->value, read->auxiliary, 0.3);
    memory.archivePage = read->id;
    RelationalChamber chamber(81u);
    const auto decision = chamber.evaluate(flute, tuba, voice, memory, 7);
    require(decision.sourcePage == read->id);
    require(decision.manifestedBodies == 4);
    const auto rereadPage = archive.latest(ModuleDomain::Memory);
    require(rereadPage.has_value());
    require(rereadPage->id != page);
    require(rereadPage->parent == page);
    require(rereadPage->action == ArchiveAction::Reread);
    require(archive.count(ArchiveAction::Reread) == 1);
}

void archive_capacity_trims_oldest_pages() {
    OpenModuleArchive archive(32);
    ArchivePageId newest = 0;
    for (int bar = 0; bar < 40; ++bar) {
        newest = archive.publish(1, ModuleDomain::Memory,
                                 static_cast<double>(bar), 0.0, bar);
    }
    require(archive.size() == 32);
    require(!archive.page(1).has_value());
    const auto latest = archive.latest(ModuleDomain::Memory);
    require(latest.has_value());
    require(latest->id == newest);
}

void archive_domains_do_not_cross_readers() {
    OpenModuleArchive archive;
    archive.publish(1, ModuleDomain::Rhythm, 0.2, 0.0, 1);
    archive.publish(2, ModuleDomain::Memory, 0.8, 0.0, 2);
    const auto memory = archive.latest(ModuleDomain::Memory);
    const auto rhythm = archive.latest(ModuleDomain::Rhythm);
    require(memory.has_value() && rhythm.has_value());
    require(memory->value == 0.8);
    require(rhythm->value == 0.2);
    require(!archive.readLatest(9, ModuleDomain::Pitch, 3).has_value());
}

void response_policy_is_bounded_by_action() {
    ChamberDecision none;
    none.manifestedBodies = 4;
    none.consequence = false;
    const auto neutral = ChamberResponse::fromDecision(none);
    require(neutral.fluteGain == 1.0);
    require(neutral.tubaPressure == 1.0);
    require(neutral.voiceGain == 1.0);
    require(neutral.spatialPolarity == 0.0);
    require(neutral.valid());

    const ChamberAction actions[] = {
        ChamberAction::Cite, ChamberAction::Resist,
        ChamberAction::Deform, ChamberAction::Scar
    };
    for (const auto action : actions) {
        ChamberDecision decision;
        decision.action = action;
        decision.manifestedBodies = 4;
        decision.consequence = true;
        decision.pressure = 1.0;
        decision.interval = 24.0;
        const auto response = ChamberResponse::fromDecision(decision);
        require(response.fluteGain >= 0.78 && response.fluteGain <= 1.22);
        require(response.tubaPressure >= 0.68 && response.tubaPressure <= 1.32);
        require(response.voiceGain >= 0.76 && response.voiceGain <= 1.12);
        require(std::isfinite(response.fluteGain));
        require(std::isfinite(response.tubaPressure));
        require(std::isfinite(response.voiceGain));
        require(response.valid());
    }
}

void response_policy_is_deterministic() {
    ChamberDecision decision;
    decision.action = ChamberAction::Deform;
    decision.manifestedBodies = 3;
    decision.consequence = true;
    decision.pressure = 0.73;
    decision.interval = -11.0;
    const auto first = ChamberResponse::fromDecision(decision);
    const auto second = ChamberResponse::fromDecision(decision);
    require(first.valid() && second.valid());
    require(first.fluteGain == second.fluteGain);
    require(first.tubaPressure == second.tubaPressure);
    require(first.voiceGain == second.voiceGain);
    require(first.spatialPolarity == second.spatialPolarity);
}

void ab_report_is_neutral_for_same_response() {
    ChamberResponse response;
    const auto report = ChamberABReport::compare(response, response);
    require(report.valid());
    require(!report.changed());
    require(report.fluteGainDelta == 0.0);
    require(report.spatialPolarityDelta == 0.0);
}

void ab_report_exposes_causal_difference() {
    ChamberDecision decision;
    decision.action = ChamberAction::Resist;
    decision.manifestedBodies = 4;
    decision.consequence = true;
    decision.pressure = 0.9;
    decision.interval = 8.0;
    const auto baseline = ChamberResponse{};
    const auto variant = ChamberResponse::fromDecision(decision);
    const auto report = ChamberABReport::compare(baseline, variant);
    require(report.valid());
    require(report.changed());
    require(report.voiceGainDelta < 0.0);
    require(report.spatialPolarityDelta < 0.0);
}

void ab_report_rejects_invalid_side() {
    ChamberResponse invalid;
    invalid.fluteGain = 2.0;
    const auto report = ChamberABReport::compare(invalid, ChamberResponse{});
    require(!report.valid());
    require(std::isfinite(report.fluteGainDelta));
}

void ab_report_honors_tolerance() {
    ChamberResponse baseline;
    ChamberResponse variant = baseline;
    variant.fluteGain += 5.0e-13;
    const auto report = ChamberABReport::compare(baseline, variant);
    require(report.valid());
    require(!report.changed());
    require(report.changed(1.0e-13));
}

} // namespace

int main() {
    deterministic_and_provenant();
    action_names_cover_contract();
    silence_does_not_invent_relation();
    memory_and_resistance_are_bar_rate();
    completed_section_snapshot_reaches_chamber_after_reset();
    disabled_is_neutral();
    archive_read_preserves_provenance();
    chamber_reuses_memory_page_identity();
    archive_capacity_trims_oldest_pages();
    archive_domains_do_not_cross_readers();
    response_policy_is_bounded_by_action();
    response_policy_is_deterministic();
    ab_report_is_neutral_for_same_response();
    ab_report_exposes_causal_difference();
    ab_report_rejects_invalid_side();
    ab_report_honors_tolerance();
    return 0;
}
