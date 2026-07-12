// DiagnosticEnvelope.cpp
//
// Implementation of DiagnosticEnvelopeBuilder. The builder enforces
// that every DiagnosticEnvelope produced by this branch has all
// canonical required fields populated before it is handed off to a
// caller. If any required field is missing, build_root() or
// build_child() throws std::logic_error — no partial envelope is
// ever returned.
//
// Canonical source of truth:
//   specs/interfaces/diagnostic-schema.md on main
//
// Stage ordering enforcement (from spec):
//   ESCALATION parent must be RECOVERY or CLASSIFICATION.
//   TERMINAL parent must be ESCALATION or RECOVERY.

#include "ash/DiagnosticEnvelope.hpp"

#include <stdexcept>
#include <utility>

namespace ash {

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::kind(DiagnosticKind k) noexcept {
    kind_ = k;
    kind_set_ = true;
    return *this;
}

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::severity(Severity s) noexcept {
    severity_ = s;
    severity_set_ = true;
    return *this;
}

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::stage(Stage s) noexcept {
    stage_ = s;
    stage_set_ = true;
    return *this;
}

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::disposition(Disposition d) noexcept {
    disposition_ = d;
    disposition_set_ = true;
    return *this;
}

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::subject(std::string_view s) {
    subject_.assign(s);
    return *this;
}

DiagnosticEnvelopeBuilder&
DiagnosticEnvelopeBuilder::parent(std::shared_ptr<DiagnosticEnvelope> p) noexcept {
    parent_ = std::move(p);
    return *this;
}

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::rule_id(std::string_view id) {
    rule_ids_.emplace_back(id);
    return *this;
}

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::summary(std::string_view s) {
    summary_.assign(s);
    return *this;
}

DiagnosticEnvelopeBuilder& DiagnosticEnvelopeBuilder::note(std::string_view n) {
    notes_.emplace_back(n);
    return *this;
}

void DiagnosticEnvelopeBuilder::validate_required_fields_() const {
    if (!kind_set_) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: diagnostic_kind not set");
    }
    if (!severity_set_) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: severity not set");
    }
    if (!stage_set_) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: stage not set");
    }
    if (!disposition_set_) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: disposition not set");
    }
    if (subject_.empty()) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: subject_reference empty");
    }
    if (rule_ids_.empty()) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: rule_ids empty");
    }
    if (summary_.empty()) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: summary empty");
    }
    if (notes_.empty()) {
        throw std::logic_error("DiagnosticEnvelopeBuilder: notes empty");
    }
    // Every rule_id and note must itself be non-empty.
    for (const auto& id : rule_ids_) {
        if (id.empty()) {
            throw std::logic_error("DiagnosticEnvelopeBuilder: rule_ids contains empty entry");
        }
    }
    for (const auto& n : notes_) {
        if (n.empty()) {
            throw std::logic_error("DiagnosticEnvelopeBuilder: notes contains empty entry");
        }
    }
}

std::shared_ptr<DiagnosticEnvelope> DiagnosticEnvelopeBuilder::build_root() {
    validate_required_fields_();

    auto envelope = std::make_shared<DiagnosticEnvelope>();
    envelope->diagnostic_kind = kind_;
    envelope->severity = severity_;
    envelope->stage = stage_;
    envelope->disposition = disposition_;
    envelope->subject_reference = subject_;
    envelope->parent_diagnostic_reference = nullptr;  // root
    envelope->rule_ids = rule_ids_;
    envelope->summary = summary_;
    envelope->notes = notes_;

    // For a root, chain_root_reference points at self. We use a
    // self-reference via shared_from_this-style assignment after the
    // envelope is constructed.
    envelope->chain_root_reference = envelope;

    return envelope;
}

std::shared_ptr<DiagnosticEnvelope> DiagnosticEnvelopeBuilder::build_child() {
    validate_required_fields_();

    if (!parent_) {
        throw std::logic_error("DiagnosticEnvelopeBuilder::build_child: parent not set");
    }

    // Stage ordering enforcement per canonical spec.
    const Stage parent_stage = parent_->stage;
    if (stage_ == Stage::ESCALATION) {
        if (parent_stage != Stage::RECOVERY && parent_stage != Stage::CLASSIFICATION) {
            throw std::logic_error(
                "DiagnosticEnvelopeBuilder::build_child: ESCALATION parent must be "
                "RECOVERY or CLASSIFICATION");
        }
    } else if (stage_ == Stage::TERMINAL) {
        if (parent_stage != Stage::ESCALATION && parent_stage != Stage::RECOVERY) {
            throw std::logic_error(
                "DiagnosticEnvelopeBuilder::build_child: TERMINAL parent must be "
                "ESCALATION or RECOVERY");
        }
    }

    auto envelope = std::make_shared<DiagnosticEnvelope>();
    envelope->diagnostic_kind = kind_;
    envelope->severity = severity_;
    envelope->stage = stage_;
    envelope->disposition = disposition_;
    envelope->subject_reference = subject_;
    envelope->parent_diagnostic_reference = parent_;
    envelope->rule_ids = rule_ids_;
    envelope->summary = summary_;
    envelope->notes = notes_;

    // Inherit chain_root_reference from the parent. If the parent's
    // chain_root_reference is null (shouldn't happen for a valid
    // parent, but defensively handle), the parent itself is treated
    // as the chain root.
    envelope->chain_root_reference =
        parent_->chain_root_reference ? parent_->chain_root_reference : parent_;

    return envelope;
}

}  // namespace ash
