#include "ash/DiagnosticEnvelope.hpp"
#include "ash/Diagnostics.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace ash {
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::kind(DiagnosticKind value) noexcept { kind_ = value; kind_set_ = true; return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::severity(Severity value) noexcept { severity_ = value; severity_set_ = true; return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::stage(Stage value) noexcept { stage_ = value; stage_set_ = true; return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::disposition(Disposition value) noexcept { disposition_ = value; disposition_set_ = true; return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::subject(std::string_view value) { subject_.assign(value); return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::parent(std::shared_ptr<DiagnosticEnvelope> value) noexcept { parent_ = std::move(value); return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::rule_id(std::string_view value) { rule_ids_.emplace_back(value); return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::summary(std::string_view value) { summary_.assign(value); return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::note(std::string_view value) { notes_.emplace_back(value); return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::details(DiagnosticObject value) { details_ = std::move(value); return *this; }
DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::occurrence_metadata(std::optional<DiagnosticObject> value) { occurrence_metadata_ = std::move(value); return *this; }

void DiagnosticEnvelopeBuilder::validate_required_fields_() const {
    if (!kind_set_ || !severity_set_ || !stage_set_ || !disposition_set_)
        throw std::logic_error("DiagnosticEnvelopeBuilder: required vocabulary field not set");
}

std::shared_ptr<DiagnosticEnvelope> DiagnosticEnvelopeBuilder::build_root() {
    validate_required_fields_();
    auto envelope = std::make_shared<DiagnosticEnvelope>();
    envelope->diagnostic_kind = kind_;
    envelope->severity = severity_;
    envelope->stage = stage_;
    envelope->disposition = disposition_;
    envelope->subject_reference = subject_;
    envelope->rule_ids = rule_ids_;
    envelope->summary = summary_;
    envelope->notes = notes_;
    envelope->details = details_;
    envelope->occurrence_metadata = occurrence_metadata_;
    envelope->diagnostic_id = Diagnostics{}.semantic_identity(*envelope);
    envelope->chain_root_diagnostic_id = envelope->diagnostic_id;
    envelope->chain_root_reference = envelope;
    if (!Diagnostics{}.validate_envelope(*envelope))
        throw std::logic_error("DiagnosticEnvelopeBuilder: invalid record content");
    return envelope;
}

std::shared_ptr<DiagnosticEnvelope> DiagnosticEnvelopeBuilder::build_child() {
    if (!parent_ || !Diagnostics{}.validate_chain(*parent_))
        throw std::logic_error("DiagnosticEnvelopeBuilder: invalid or missing parent chain");
    if (parent_->sequence_number == (std::numeric_limits<std::size_t>::max)())
        throw std::logic_error("DiagnosticEnvelopeBuilder: sequence overflow");
    // Semantic content is independent of its position in the chain.
    auto envelope = build_root();
    envelope->parent_diagnostic_reference = parent_;
    envelope->parent_diagnostic_id = parent_->diagnostic_id;
    envelope->chain_root_diagnostic_id = parent_->chain_root_diagnostic_id;
    envelope->chain_root_reference = parent_->chain_root_reference;
    envelope->sequence_number = parent_->sequence_number + 1;
    if (!Diagnostics{}.validate_chain(*envelope))
        throw std::logic_error("DiagnosticEnvelopeBuilder: invalid causal chain");
    return envelope;
}
} // namespace ash
