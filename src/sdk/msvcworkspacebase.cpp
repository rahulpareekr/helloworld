#include "msvcworkspacebase.h"
#include "manager.h"
#include "messagemanager.h"
#include "projectmanager.h"
#include "compilerfactory.h"
#include "compiler.h"

MSVCWorkspaceBase::MSVCWorkspaceBase() {
    // nothing to do
}

MSVCWorkspaceBase::~MSVCWorkspaceBase() {
    // nothing to do
}

void MSVCWorkspaceBase::registerProject(const wxString& projectID, cbProject* project) {
    // just set the initial project dependencies as empty and register the idcode
    _projects[projectID] = ProjectRecord(project);
}

void MSVCWorkspaceBase::addDependency(const wxString& projectID, const wxString& dependencyID) {
    // add the dependency to the last project
    HashProjects::iterator it = _projects.find(projectID);
    if (it != _projects.end()) {
        if (it->second._dependencyList.Index(dependencyID) == wxNOT_FOUND) // not already in
            it->second._dependencyList.Add(dependencyID);
    }
    else {
        Manager::Get()->GetMessageManager()->DebugLog(_("ERROR: project id not found: %s"), projectID.c_str());
    }
}

void MSVCWorkspaceBase::addWorkspaceConfiguration(const wxString& config) {
    _workspaceConfigurations.Add(config);
}

void MSVCWorkspaceBase::addConfigurationMatching(const wxString& projectID, const wxString& workspConfig, const wxString& projConfig) {
    //Manager::Get()->GetMessageManager()->DebugLog(_T("adding conf match: '%s' - '%s'"), workspConfig.c_str(), projConfig.c_str());
    HashProjects::iterator it = _projects.find(projectID);
    if (it != _projects.end()) {
        it->second._configurations[workspConfig] = projConfig;
    }
    else Manager::Get()->GetMessageManager()->DebugLog(_T("ERROR: project id not found: %s"), projectID.c_str());
}

void MSVCWorkspaceBase::updateProjects() {
    HashProjects::iterator projIt;
    HashProjects::iterator depIt;
    ProjectBuildTarget* targetProj = 0;
    ProjectBuildTarget* targetDep = 0;
    ProjectRecord proj;
    ProjectRecord dep;
    unsigned int i;
    unsigned int j;
    int k;

    Manager::Get()->GetMessageManager()->DebugLog(_T("Update projects"));
    for (projIt = _projects.begin(); projIt != _projects.end(); ++projIt) {
        proj = projIt->second;
        Manager::Get()->GetMessageManager()->DebugLog(_T("Project %s, %d dependencies"), proj._project->GetTitle().c_str(), proj._dependencyList.GetCount());
        for (i=0; i<proj._dependencyList.GetCount(); ++i) {
            depIt = _projects.find(proj._dependencyList[i]);
            if ( depIt != _projects.end()) { // dependency found
                dep = depIt->second;

                // no per-workspace config for msvc6, so build a fake one ;)
                if (_workspaceConfigurations.IsEmpty()) {
                    Manager::Get()->GetMessageManager()->DebugLog(_T("Workspace configurations will be generated from those of project %s"), proj._project->GetTitle().c_str());
                    for (k=0; k<proj._project->GetBuildTargetsCount(); ++k) {
                        // should be the configurations, not the build target title
                        wxString s = proj._project->GetBuildTarget(k)->GetTitle();
                        _workspaceConfigurations.Add(s);
                    }
                }

                // match target configurations
                for (j=0; j<_workspaceConfigurations.GetCount(); ++j) {
                    ConfigurationMatchings::iterator configIt;
                    wxString projConfig;
                    wxString depConfig;

                    //Manager::Get()->GetMessageManager()->DebugLog(_T("config: '%s'"), _workspaceConfigurations[j].c_str());

                    if (proj._configurations.empty()) projConfig = _workspaceConfigurations[j];
                    else {
                        configIt = proj._configurations.find(_workspaceConfigurations[j]);
                        if (configIt != proj._configurations.end()) projConfig = configIt->second;
                        else Manager::Get()->GetMessageManager()->DebugLog(_T("ERROR: could not find the matching for %s project"), proj._project->GetTitle().c_str());
                    }
                    targetProj = proj._project->GetBuildTarget(projConfig);

                    if (dep._configurations.empty()) depConfig = _workspaceConfigurations[j];
                    else {
                        configIt = dep._configurations.find(_workspaceConfigurations[j]);
                        if (configIt != dep._configurations.end()) depConfig = configIt->second;
                        else Manager::Get()->GetMessageManager()->DebugLog(_T("ERROR: could not find the matching for %s project"), dep._project->GetTitle().c_str());
                    }
                    targetDep = dep._project->GetBuildTarget(depConfig);

                    if ((targetDep==0) || (targetProj==0)) {
                        Manager::Get()->GetMessageManager()->DebugLog(_T("ERROR: could not find targets"));
                        return;
                    }

                    Manager::Get()->GetMessageManager()->DebugLog(_T("Match '%s' to '%s'"), targetProj->GetFullTitle().c_str(), targetDep->GetFullTitle().c_str());

                    // now, update dependencies
                    TargetType type = targetDep->GetTargetType();
                    wxFileName fname;
                    if (type==ttDynamicLib) {
                        // targetDep->GetStaticLibFilename() do not work since it uses the filename instead of output filename
                        Compiler* compiler = CompilerFactory::Compilers[ depIt->second._project->GetCompilerIndex()];
                        wxString prefix = compiler->GetSwitches().libPrefix;
                        wxString suffix = compiler->GetSwitches().libExtension;
                        fname = targetDep->GetOutputFilename();
                        fname.SetName(prefix + fname.GetName());
                        fname.SetExt(suffix);
                    }
                    else if (type==ttStaticLib) fname = targetDep->GetOutputFilename();
                    targetProj->AddLinkLib(fname.GetFullPath());
                    targetProj->AddTargetDep(targetDep);
                    // TO REMOVE
                    wxString deps = targetProj->GetExternalDeps();
                    deps <<fname.GetFullPath() << _T(';');
                    targetProj->SetExternalDeps(deps);
                    // ---------
               }
            }
            else {
                Manager::Get()->GetMessageManager()->DebugLog(_T("ERROR: dependency not found %s"), proj._dependencyList[i].c_str());
            }
        }
    }

    //target->AddCommandsBeforeBuild(const wxString& command);
}
