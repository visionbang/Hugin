// -*- c-basic-offset: 4 -*-

/** @file CPDetectorConfig.cpp
 *
 *  @brief implementation of CPDetectorSetting, CPDetectorConfig and CPDetectorDialog classes, 
 *         which are for storing and changing settings of different CP detectors
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "hugin/CPDetectorConfig.h"
#include <config.h>
#include "base_wx/huginConfig.h"
#include "hugin/config_defaults.h"
#include <hugin/CPDetectorConfig_default.h>
#include "hugin/huginApp.h"

void CPDetectorConfig::Read(wxConfigBase *config)
{
    settings.Clear();
    int count=config->Read(wxT("/AutoPano/AutoPanoCount"),0l);
    default_generator=config->Read(wxT("/AutoPano/Default"),0l);
    if(count>0)
    {
        for(int i=0;i<count;i++)
            ReadIndex(config, i);
    };
    if(settings.GetCount()==0)
        ResetToDefault();
    if(default_generator>=settings.GetCount())
        default_generator=0;
};

void CPDetectorConfig::ReadIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    if(config->HasGroup(path))
    {
        CPDetectorSetting* gen=new CPDetectorSetting;
        gen->Read(config,path);
        settings.Add(gen);
        if(i==default_generator)
            default_generator=settings.Index(*gen,true);
    };
};

void CPDetectorConfig::Write(wxConfigBase *config)
{
    int count=settings.Count();
    config->Write(wxT("/AutoPano/AutoPanoCount"),count);
    config->Write(wxT("/AutoPano/Default"),(int)default_generator);
    if(count>0)
    {
        for(int i=0;i<count;i++)
            WriteIndex(config, i);
    };
};

void CPDetectorConfig::WriteIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    settings[i].Write(config,path);
};

void CPDetectorConfig::ResetToDefault()
{
    settings.Clear();
    int maxIndex=sizeof(default_cpdetectors)/sizeof(cpdetector_default)-1;
    for(int i=0;i<=maxIndex;i++)
        settings.Add(new CPDetectorSetting(i));
    default_generator=0;
};

void CPDetectorConfig::FillControl(wxControlWithItems *control,bool select_default,bool show_default)
{
    control->Clear();
    for(unsigned int i=0;i<settings.GetCount();i++)
    {
        wxString s=settings[i].GetCPDetectorDesc();
        if(show_default && i==default_generator)
            s=s+wxT(" (")+_("Default")+wxT(")");
        control->Append(s);
    };
    if(select_default)
        control->SetSelection(default_generator);
};

void CPDetectorConfig::Swap(int index)
{
    CPDetectorSetting* setting=settings.Detach(index);
    settings.Insert(setting,index+1);
    if(default_generator==index)
        default_generator=index+1;
    else 
        if(default_generator==index+1)
            default_generator=index;
};

void CPDetectorConfig::SetDefaultGenerator(unsigned int new_default_generator)
{
    if(new_default_generator<GetCount())
        default_generator=new_default_generator;
    else
        default_generator=0;
};

#include <wx/arrimpl.cpp> 
WX_DEFINE_OBJARRAY(ArraySettings);

CPDetectorSetting::CPDetectorSetting(int new_type)
{
    custom=true;
    if(new_type>=0)
    {
        int maxIndex=sizeof(default_cpdetectors)/sizeof(cpdetector_default)-1;
        if (new_type<=maxIndex)
        {
            type=default_cpdetectors[new_type].type;
            desc=default_cpdetectors[new_type].desc;
            prog=default_cpdetectors[new_type].prog;
            args=default_cpdetectors[new_type].args;
#ifdef MAC_SELF_CONTAINED_BUNDLE
            custom=default_cpdetectors[new_type].custom;
#else
            custom=true;
#endif
        };
    };
};

void CPDetectorSetting::Read(wxConfigBase *config, wxString path)
{
    type=config->Read(path+wxT("/Type"),default_cpdetectors[0].type);
    desc=config->Read(path+wxT("/Description"),default_cpdetectors[0].desc);
    prog=config->Read(path+wxT("/Program"),default_cpdetectors[0].prog);
    args=config->Read(path+wxT("/Arguments"),default_cpdetectors[0].args);
#if defined MAC_SELF_CONTAINED_BUNDLE
    custom=config->Read(path+wxT("/AutopanoExeCustom"),(bool*)default_cpdetectors[0].custom);
#else
    custom=true;
#endif
};

void CPDetectorSetting::Write(wxConfigBase *config, wxString path)
{
    config->Write(path+wxT("/Type"),type);
    config->Write(path+wxT("/Description"),desc);
    config->Write(path+wxT("/Program"),prog);
    config->Write(path+wxT("/Arguments"),args);
#if defined MAC_SELF_CONTAINED_BUNDLE
    config->Write(path+wxT("/AutopanoExeCustom"),custom);
#endif
};

// dialog for showing settings of one autopano setting

BEGIN_EVENT_TABLE(CPDetectorDialog,wxDialog)
    EVT_BUTTON(wxID_OK, CPDetectorDialog::OnOk)
    EVT_BUTTON(XRCID("prefs_cpdetector_program_select"),CPDetectorDialog::OnSelectPath)
    EVT_CHECKBOX(XRCID("prefs_cpdetector_custom"),CPDetectorDialog::OnCustomCPDetector)
END_EVENT_TABLE()

CPDetectorDialog::CPDetectorDialog(wxWindow* parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("cpdetector_dialog"));
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    //restore frame position and size
    RestoreFramePosition(this,wxT("CPDetectorDialog"));

    m_edit_desc = XRCCTRL(*this, "prefs_cpdetector_desc", wxTextCtrl);
    m_edit_prog = XRCCTRL(*this, "prefs_cpdetector_program", wxTextCtrl);
    m_edit_args = XRCCTRL(*this, "prefs_cpdetector_args", wxTextCtrl);
    m_cpdetector_type = XRCCTRL(*this, "prefs_cpdetector_type", wxChoice);
    m_cpdetector_type->SetSelection(1);
    m_custom_cpdetector = XRCCTRL(*this, "prefs_cpdetector_custom", wxCheckBox);
#ifdef MAC_SELF_CONTAINED_BUNDLE
    m_custom_cpdetector->SetValue(FALSE);
#else
    m_custom_cpdetector->SetValue(TRUE);
#endif
    EnableControls(m_custom_cpdetector->GetValue());
#ifndef MAC_SELF_CONTAINED_BUNDLE
    m_custom_cpdetector->Hide();
#endif
};

CPDetectorDialog::~CPDetectorDialog()
{
    StoreFramePosition(this,wxT("CPDetectorDialog"));
};

void CPDetectorDialog::OnCustomCPDetector(wxCommandEvent &e)
{
    EnableControls(e.IsChecked());
};

void CPDetectorDialog::EnableControls(bool state)
{
    m_edit_prog->Enable(state);
    XRCCTRL(*this, "prefs_cpdetector_program_select", wxButton)->Enable(state);
};

void CPDetectorDialog::OnOk(wxCommandEvent & e)
{
#ifdef __WXMAC__
    if(m_cpdetector_type->GetSelection()==0)
    {
        wxMessageBox(_("Autopano from http://autopano.kolor.com is not available for OSX"), 
                     _("Using Autopano-Sift instead"),wxOK|wxICON_EXCLAMATION, this); 
        m_cpdetector_type->SetSelection(1);
    };
#endif
    bool valid=true;
    valid=valid && (m_edit_desc->GetValue().Trim().Len()>0);
    if (m_custom_cpdetector->IsChecked())
        valid=valid && (m_edit_prog->GetValue().Trim().Len()>0);
    valid=valid && (m_edit_args->GetValue().Trim().Len()>0);
    if(valid)        
        this->EndModal(wxOK);
    else
        wxMessageBox(_("At least one input field is empty.\nPlease check your inputs."),
            _("Warning"),wxOK | wxICON_ERROR,this);
};

void CPDetectorDialog::UpdateFields(CPDetectorConfig* cpdet_config,int index)
{
    m_edit_desc->SetValue(cpdet_config->settings[index].GetCPDetectorDesc());
    m_edit_prog->SetValue(cpdet_config->settings[index].GetProg());
    m_edit_args->SetValue(cpdet_config->settings[index].GetArgs());
    m_cpdetector_type->SetSelection(cpdet_config->settings[index].GetType());
    m_custom_cpdetector->SetValue(cpdet_config->settings[index].GetCPDetectorExeCustom());
    EnableControls(m_custom_cpdetector->GetValue());
};

void CPDetectorDialog::UpdateSettings(CPDetectorConfig* cpdet_config,int index)
{
    cpdet_config->settings[index].SetCPDetectorDesc(m_edit_desc->GetValue().Trim());
    cpdet_config->settings[index].SetProg(m_edit_prog->GetValue().Trim());
    cpdet_config->settings[index].SetArgs(m_edit_args->GetValue().Trim());
    cpdet_config->settings[index].SetType(m_cpdetector_type->GetSelection());
    cpdet_config->settings[index].SetCPDetectorExeCustom(m_custom_cpdetector->GetValue());
};

void CPDetectorDialog::OnSelectPath(wxCommandEvent &e)
{
    wxFileName executable(m_edit_prog->GetValue());
    wxFileDialog dlg(this,_("Select Autopano program"), executable.GetPath(), executable.GetFullName(),
#ifdef __WXMSW__
             _("Executables (*.exe,*.vbs,*.cmd, *.bat)|*.exe;*.vbs;*.cmd;*.bat"),
#else
             wxT(""),
#endif
             wxOPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
        m_edit_prog->SetValue(dlg.GetPath());
};