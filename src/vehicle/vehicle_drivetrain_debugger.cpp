#include "vehicle_drivetrain_debugger.h"
#include "godot_cpp/classes/graph_edit.hpp"
#include "godot_cpp/classes/graph_node.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/h_separator.hpp"
#include "godot_cpp/classes/label.hpp"
#include "vehicle/shaft.h"
#include "vehicle/vehicle.h"

void LNVehicleDrivetrainDebugger::update() {
    for (auto &[name, value] : per_shaft_nodes) {
        const String debug_text = value.shaft->get_debug_text();
        if (debug_text.is_empty()) {
            continue;
        }

        value.separator->show();
        value.debug_label->show();
        value.debug_label->set_text(debug_text);
    }
}

void LNVehicleDrivetrainDebugger::update_tree(LNVehicle *p_vehicle) {
    for (KeyValue<StringName, PerShaftNode> &kv : per_shaft_nodes) {
        graph_edit->remove_child(kv.value.graph_node);
        kv.value.graph_node->queue_free();
    }

    per_shaft_nodes.clear();

    graph_edit->add_valid_connection_type(0, 0);

    for (auto [name, shaft] : p_vehicle->shafts) {
        GraphNode *node = memnew(GraphNode);
        node->set_name(name);

        graph_edit->add_child(node);

        Vector<Label*> out_labels;
        Label* in_label = nullptr;
        
        for (int i = 0; i < MAX(shaft->get_output_count(), 1); i++) {
            HBoxContainer *container = memnew(HBoxContainer);
            node->add_child(container);
            if (i == 0 && shaft->has_input()) {
                node->set_slot_enabled_left(i, true);
                Label *label = memnew(Label);
                label->set_text("Input");
                container->add_child(label);
                in_label = label;
            }

            if (i < shaft->get_output_count()) {
                Label *label = memnew(Label);
                label->set_text(vformat("Output %d", i));
                label->set_h_size_flags(Control::SizeFlags::SIZE_EXPAND | Control::SizeFlags::SIZE_SHRINK_END);
                container->add_child(label);
                node->set_slot_enabled_right(i, i < shaft->get_output_count());
                out_labels.push_back(label);
            }
        }

        node->set_title(shaft->get_debugger_display_name());
        Label *debug_label = memnew(Label);
        HSeparator *separator = memnew(HSeparator);
        node->add_child(separator);
        per_shaft_nodes.insert(name, {
            .input_label = in_label,
            .debug_label = debug_label,
            .output_labels = out_labels,
            .graph_node = node,
            .separator = separator,
            .shaft = shaft
        });
        node->add_child(debug_label);

        debug_label->hide();
        separator->hide();
    }

    for (auto [name, shaft] : p_vehicle->shafts) {
        for (int i = 0; i < shaft->get_output_count(); i++) {
            if (LNVehicleShaft *child = shaft->get_child(i); child != nullptr) {
                graph_edit->connect_node(name, i, child->get_name(), 0);
            }
        }
    }

    graph_edit->arrange_nodes();
}

void LNVehicleDrivetrainDebugger::_ready() {
    graph_edit = memnew(GraphEdit);
    add_child(graph_edit);
}
