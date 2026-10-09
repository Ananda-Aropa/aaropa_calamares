#!/bin/bash
# Skip the partition page when the existinginstall page (aaropa-calamares-extensions-bass)
# chose to upgrade an existing Bass OS install: nothing is partitioned or formatted, the
# existinginstall exec job assigns mount points afterwards, and the summary page shows
# the upgrade instead of an empty "Partitions" section.

python3 - src/modules/partition/PartitionViewStep.cpp <<'EOF'
import sys

path = sys.argv[1]
src = open(path).read()


def sub(old, new):
    global src
    if src.count(old) != 1:
        sys.exit("005-module-partition-upgrade-skip: anchor not found once: " + old.splitlines()[0])
    src = src.replace(old, new)


sub('#include "JobQueue.h"\n',
    '#include "JobQueue.h"\n#include "ViewManager.h"\n#include "jobs/FillGlobalStorageJob.h"\n')

sub('#include <QtConcurrent/QtConcurrent>\n', '''#include <QtConcurrent/QtConcurrent>

static bool
bassUpgradeSelected()
{
    auto* gs = Calamares::JobQueue::instance() ? Calamares::JobQueue::instance()->globalStorage() : nullptr;
    return gs && gs->value( "bassUpgrade" ).toMap().value( "enabled" ).toBool();
}
''')

# The existinginstall page describes an upgrade on the summary page.
sub('PartitionViewStep::prettyStatus() const\n{\n', '''PartitionViewStep::prettyStatus() const
{
    if ( bassUpgradeSelected() )
    {
        return QString();
    }
''')

sub('PartitionViewStep::createSummaryWidget() const\n{\n', '''PartitionViewStep::createSummaryWidget() const
{
    if ( bassUpgradeSelected() )
    {
        return nullptr;
    }
''')

sub('PartitionViewStep::isAtEnd() const\n{\n', '''PartitionViewStep::isAtEnd() const
{
    if ( bassUpgradeSelected() )
    {
        return true;
    }
''')

sub('    m_config->fillGSSecondaryConfiguration();\n', '''    m_config->fillGSSecondaryConfiguration();

    if ( bassUpgradeSelected() )
    {
        auto* gs = Calamares::JobQueue::instance()->globalStorage();
        auto* vm = Calamares::ViewManager::instance();
        if ( gs->contains( "_partition_skipped" ) )
        {
            gs->remove( "_partition_skipped" );
            vm->back();
        }
        else
        {
            // Drop changes made on an earlier visit (e.g. "Erase disk" before switching to upgrade).
            if ( m_core->isDirty() )
            {
                m_core->revertAllDevices();
            }
            gs->insert( "_partition_skipped", true );
            vm->next();
        }
        return;
    }
''')

sub('PartitionViewStep::onLeave()\n{\n', '''PartitionViewStep::onLeave()
{
    if ( bassUpgradeSelected() )
    {
        return;
    }
''')

sub('    return m_core->jobs( m_config );\n', '''    Calamares::JobList jobs = m_core->jobs( m_config );
    if ( bassUpgradeSelected() )
    {
        // Upgrade never changes the partition table: only publish the device list.
        Calamares::JobList fill;
        for ( const auto& job : jobs )
        {
            if ( dynamic_cast< FillGlobalStorageJob* >( job.data() ) )
            {
                fill << job;
            }
        }
        return fill;
    }
    return jobs;
''')

open(path, "w").write(src)
EOF
