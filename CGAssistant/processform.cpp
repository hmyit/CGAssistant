#include "processform.h"
#include "ui_processform.h"
#include <QMessageBox>
#include <Windows.h>

static HANDLE s_AttachMutex = NULL;

ProcessForm::ProcessForm(CProcessWorker *worker, QWidget *parent) :
    QWidget(parent), m_worker(worker),
    ui(new Ui::ProcessForm)
{
    ui->setupUi(this);

    m_hwnd = NULL;
    m_sync = true;

    m_model = new CProcessTableModel(ui->tableView_process);
    ui->tableView_process->setModel(m_model);
    ui->tableView_process->setColumnWidth(0, 60);
    ui->tableView_process->setColumnWidth(1, 200);
    ui->tableView_process->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Fixed);
    ui->tableView_process->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
    ui->tableView_process->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);

    connect(this, &ProcessForm::QueueAttachProcess, m_worker, &CProcessWorker::OnQueueAttachProcess, Qt::QueuedConnection);
    connect(m_worker, &CProcessWorker::NotifyAttachProcessOk, this, &ProcessForm::OnNotifyAttachProcessOk, Qt::QueuedConnection);
    connect(m_worker, &CProcessWorker::NotifyAttachProcessFailed, this, &ProcessForm::OnNotifyAttachProcessFailed, Qt::QueuedConnection);
    connect(m_worker, &CProcessWorker::NotifyQueryProcess, this, &ProcessForm::OnNotifyQueryProcess, Qt::QueuedConnection);


}

ProcessForm::~ProcessForm()
{
    delete ui;
}

void ProcessForm::OnCloseWindow()
{
    ReleaseMutex(s_AttachMutex);
    CloseHandle(s_AttachMutex);
}

void ProcessForm::OnNotifyAttachProcessOk(quint32 ProcessId, quint32 port, quint32 hwnd)
{
    m_hwnd = (HWND)hwnd;
}

void ProcessForm::OnNotifyAttachProcessFailed(quint32 ProcessId, int errorCode, QString errorString)
{
    QMessageBox::critical(this, tr("Error"), tr("Failed to attach to process.\n%1").arg(errorString), QMessageBox::Ok, 0);
}

void ProcessForm::OnNotifyQueryProcess(CProcessItemList list)
{
    for(int i = 0;i < list.size(); ++i)
    {
        const CProcessItemPtr &newItem = list.at(i);
        if(i < m_model->rowCount())
        {
            CProcessItem *item = m_model->ItemFromIndex(m_model->index(i, 0));

            item->m_ProcessId = newItem->m_ProcessId;
            item->m_ThreadId = newItem->m_ThreadId;
            item->m_Title = newItem->m_Title;
            item->m_hWnd = newItem->m_hWnd;
            item->m_bAttached = newItem->m_bAttached;

            m_model->dataChanged(m_model->index(i, 0), m_model->index(i, 2));
        } else {
            m_model->appendRow(newItem);
        }
    }
    if(m_model->rowCount() > list.size())
    {
        m_model->removeRows(list.size(), m_model->rowCount() - list.size());
    }
}

void ProcessForm::on_pushButton_attach_clicked()
{
    if(!ui->tableView_process->currentIndex().isValid())\
        return;

    CProcessItem *item = m_model->ItemFromIndex(ui->tableView_process->currentIndex());
    if(!item)
        return;
    QueueAttachProcess( (quint32)item->m_ProcessId, (quint32)item->m_ThreadId, (quint32)item->m_hWnd, QString("cgahook.dll") );
}

void ProcessForm::on_checkBox_syncwnd_stateChanged(int checked)
{
    m_sync = checked ? true : false;
}

void ProcessForm::OnNotifyChangeWindow(Qt::WindowStates st)
{
    if(m_sync && m_hwnd && IsWindow(m_hwnd)){
        if(st == Qt::WindowState::WindowMinimized)
        {
            ShowWindow(m_hwnd, SW_MINIMIZE);
        }
        else{
            HDC hDcWnd=::GetDC(::GetDesktopWindow());

            HPEN  hPen=::CreatePen(PS_SOLID,3,RGB(255,0,0));
            SelectObject(hDcWnd,hPen);
            RECT rc;
            GetWindowRect(m_hwnd, &rc);

            MoveToEx(hDcWnd,rc.left,rc.top, NULL);
            LineTo(hDcWnd,rc.right, rc.top);
            LineTo(hDcWnd,rc.right, rc.bottom);
            LineTo(hDcWnd,rc.left, rc.bottom);

            DeleteObject(hPen);
            ReleaseDC(::GetDesktopWindow(),hDcWnd);

            ShowWindow(m_hwnd, SW_SHOWNORMAL);
        }
    }
}
