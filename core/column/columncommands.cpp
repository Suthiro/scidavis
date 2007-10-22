/***************************************************************************
    File                 : columncommands.cpp
    Project              : SciDAVis
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Tilman Hoener zu Siederdissen,
    Email (use @ for *)  : thzs*gmx.net
    Description          : Commands to change columns
                           
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#include "columncommands.h"

///////////////////////////////////////////////////////////////////////////
// class ColumnSetModeCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetModeCmd::ColumnSetModeCmd(shared_ptr<ColumnPrivate> col, SciDAVis::ColumnMode mode, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_mode(mode)
{
	setText(QObject::tr("change mode of column %1").arg(col->columnLabel()));
	d_undone = false;
	d_executed = false;
}

ColumnSetModeCmd::~ColumnSetModeCmd()
{
	if(d_undone)
	{
		if(d_new_data != d_old_data)
		{
			if(d_new_type == SciDAVis::TypeDouble)
				delete static_cast< QVector<double>* >(d_new_data);
			else if(d_new_type == SciDAVis::TypeQString)
				delete static_cast< QStringList* >(d_new_data);
			else if(d_new_type == SciDAVis::TypeQDateTime)
				delete static_cast< QList<QDateTime>* >(d_new_data);
		}
	}
	else
	{
		if(d_new_data != d_old_data)
		{
			if(d_old_type == SciDAVis::TypeDouble)
				delete static_cast< QVector<double>* >(d_old_data);
			else if(d_old_type == SciDAVis::TypeQString)
				delete static_cast< QStringList* >(d_old_data);
			else if(d_old_type == SciDAVis::TypeQDateTime)
				delete static_cast< QList<QDateTime>* >(d_old_data);
		}
	}

}

void ColumnSetModeCmd::redo()
{
	if(!d_executed)
	{
		// save old values
		d_old_mode = d_col->columnMode();	
		d_old_type = d_col->dataType();
		d_old_data = d_col->dataPointer();
		d_old_in_filter = d_col->inputFilter();
		d_old_out_filter = d_col->outputFilter();
		d_old_validity = d_col->validityAttribute();

		// do the conversion
		d_col->setColumnMode(d_mode);

		// save new values
		d_new_type = d_col->dataType();
		d_new_data = d_col->dataPointer();
		d_new_in_filter = d_col->inputFilter();
		d_new_out_filter = d_col->outputFilter();
		d_new_validity = d_col->validityAttribute();
		d_executed = true;
	}
	else
	{
		// set to saved new values
		d_col->replaceModeData(d_mode, d_new_type, d_new_data, d_new_in_filter, d_new_out_filter, d_new_validity);
	}
	d_undone = false;
}

void ColumnSetModeCmd::undo()
{
	// reset to old values
	d_col->replaceModeData(d_old_mode, d_old_type, d_old_data, d_old_in_filter, d_old_out_filter, d_old_validity);

	d_undone = true;
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetModeCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnFullCopyCmd
///////////////////////////////////////////////////////////////////////////
	ColumnFullCopyCmd::ColumnFullCopyCmd(shared_ptr<ColumnPrivate> col, const AbstractColumn * src, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_src(src)
{
	setText(QObject::tr("copy values into column %1").arg(col->columnLabel()));
}

	ColumnFullCopyCmd::ColumnFullCopyCmd(shared_ptr<ColumnPrivate> col, shared_ptr<AbstractColumn> src, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_src(src.get())
{
	setText(QObject::tr("copy values into column %1").arg(col->columnLabel()));
}

ColumnFullCopyCmd::~ColumnFullCopyCmd()
{
}

void ColumnFullCopyCmd::redo()
{
	if(d_backup == 0)
	{
		d_backup = shared_ptr<ColumnPrivate>(new ColumnPrivate(0, d_src->columnMode()));
		d_backup->copy(d_col.get());
		d_col->copy(d_src);
	}
	else
	{
		// swap data + validity of orig. column and backup
		IntervalAttribute<bool> val_temp = d_col->validityAttribute();
		void * data_temp = d_col->dataPointer();
		d_col->replaceData(d_backup->dataPointer(), d_backup->validityAttribute());
		d_backup->replaceData(data_temp, val_temp);
	}
}

void ColumnFullCopyCmd::undo()
{
	// swap data + validity of orig. column and backup
	IntervalAttribute<bool> val_temp = d_col->validityAttribute();
	void * data_temp = d_col->dataPointer();
	d_col->replaceData(d_backup->dataPointer(), d_backup->validityAttribute());
	d_backup->replaceData(data_temp, val_temp);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnFullCopyCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnPartialCopyCmd
///////////////////////////////////////////////////////////////////////////
	ColumnPartialCopyCmd::ColumnPartialCopyCmd(shared_ptr<ColumnPrivate> col, const AbstractColumn * src, int src_start, int dest_start, int num_rows, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_src(src), d_src_start(src_start), d_dest_start(dest_start), d_num_rows(num_rows)
{
	setText(QObject::tr("copy values into column %1").arg(col->columnLabel()));
}

	ColumnPartialCopyCmd::ColumnPartialCopyCmd(shared_ptr<ColumnPrivate> col, shared_ptr<AbstractColumn> src, int src_start, int dest_start, int num_rows, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_src(src.get()), d_src_start(src_start), d_dest_start(dest_start), d_num_rows(num_rows)
{
	setText(QObject::tr("copy values into column %1").arg(col->columnLabel()));
}

ColumnPartialCopyCmd::~ColumnPartialCopyCmd()
{
}

void ColumnPartialCopyCmd::redo()
{
	if(d_src_backup == 0)
	{
		// copy the relevant rows of source and destination column into backup columns
		d_src_backup = shared_ptr<ColumnPrivate>(new ColumnPrivate(0, d_col->columnMode()));
		d_src_backup->copy(d_src, d_src_start, 0, d_num_rows);
		d_col_backup = shared_ptr<ColumnPrivate>(new ColumnPrivate(0, d_col->columnMode()));
		d_col_backup->copy(d_col.get(), d_dest_start, 0, d_num_rows);
	}
	d_col->copy(d_src_backup.get(), 0, d_dest_start, d_num_rows);
}

void ColumnPartialCopyCmd::undo()
{
	d_col->copy(d_col_backup.get(), 0, d_dest_start, d_num_rows);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnPartialCopyCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnInsertEmptyRowsCmd
///////////////////////////////////////////////////////////////////////////
	ColumnInsertEmptyRowsCmd::ColumnInsertEmptyRowsCmd(shared_ptr<ColumnPrivate> col, int before, int count, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_before(before), d_count(count)
{
	setText(QObject::tr("insert rows into column %1").arg(col->columnLabel()));
}

ColumnInsertEmptyRowsCmd::~ColumnInsertEmptyRowsCmd()
{
}

void ColumnInsertEmptyRowsCmd::redo()
{
	d_col->insertEmptyRows(d_before, d_count);
}

void ColumnInsertEmptyRowsCmd::undo()
{
	d_col->removeRows(d_before, d_count);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnInsertEmptyRowsCmd
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// class ColumnRemoveRowsCmd
///////////////////////////////////////////////////////////////////////////
	ColumnRemoveRowsCmd::ColumnRemoveRowsCmd(shared_ptr<ColumnPrivate> col, int first, int count, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_first(first), d_count(count)
{
	setText(QObject::tr("remove rows from column %1").arg(col->columnLabel()));
}

ColumnRemoveRowsCmd::~ColumnRemoveRowsCmd()
{
}

void ColumnRemoveRowsCmd::redo()
{
	if(d_backup == 0)
	{
		if(d_first + d_count > d_col->rowCount()) 
			d_data_row_count = d_col->rowCount() - d_first;
		else
			d_data_row_count = d_count;

		d_backup = shared_ptr<ColumnPrivate>(new ColumnPrivate(0, d_col->columnMode()));
		d_backup->copy(d_col.get(), d_first, 0, d_data_row_count);
		d_masking = d_col->maskingAttribute();
		d_formulas = d_col->formulaAttribute();
	}
	d_col->removeRows(d_first, d_count);
}

void ColumnRemoveRowsCmd::undo()
{
	d_col->insertEmptyRows(d_first, d_count);
	d_col->copy(d_backup.get(), 0, d_first, d_data_row_count);
	if(d_data_row_count < d_count)
		d_col->resizeTo(d_col->rowCount() - (d_count - d_data_row_count));
	d_col->replaceMasking(d_masking);
	d_col->replaceFormulas(d_formulas);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnRemoveRowsCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnSetPlotDesignationCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetPlotDesignationCmd::ColumnSetPlotDesignationCmd( shared_ptr<ColumnPrivate> col, SciDAVis::PlotDesignation pd , QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_new_pd(pd)
{
	setText(QObject::tr("set plot designation for column %1").arg(col->columnLabel()));
}

ColumnSetPlotDesignationCmd::~ColumnSetPlotDesignationCmd()
{
}

void ColumnSetPlotDesignationCmd::redo()
{
	d_old_pd = d_col->plotDesignation();
	d_col->setPlotDesignation(d_new_pd);
}

void ColumnSetPlotDesignationCmd::undo()
{
	d_col->setPlotDesignation(d_old_pd);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetPlotDesignationCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnClearCmd
///////////////////////////////////////////////////////////////////////////
	ColumnClearCmd::ColumnClearCmd(shared_ptr<ColumnPrivate> col, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col)
{
	setText(QObject::tr("clear column %1").arg(col->columnLabel()));
	d_empty_data = 0;
	d_undone = false;
}

ColumnClearCmd::~ColumnClearCmd()
{
	if(d_undone)
	{
		if(d_type == SciDAVis::TypeDouble)
			delete static_cast< QVector<double>* >(d_empty_data);
		else if(d_type == SciDAVis::TypeQString)
			delete static_cast< QStringList* >(d_empty_data);
		else if(d_type == SciDAVis::TypeQDateTime)
			delete static_cast< QList<QDateTime>* >(d_empty_data);
	}
	else
	{
		if(d_type == SciDAVis::TypeDouble)
			delete static_cast< QVector<double>* >(d_data);
		else if(d_type == SciDAVis::TypeQString)
			delete static_cast< QStringList* >(d_data);
		else if(d_type == SciDAVis::TypeQDateTime)
			delete static_cast< QList<QDateTime>* >(d_data);
	}
}

void ColumnClearCmd::redo()
{
	if(!d_empty_data)
	{
		d_type = d_col->dataType();
		switch(d_type)
		{
			case SciDAVis::TypeDouble:
				d_empty_data = new QVector<double>();
				break;
			case SciDAVis::TypeQDateTime:
				d_empty_data = new QList<QDateTime>();
				break;
			case SciDAVis::TypeQString:
				d_empty_data = new QStringList();
				break;
		}
		d_data = d_col->dataPointer();
		d_validity = d_col->validityAttribute();
	}
	d_col->replaceData(d_empty_data, IntervalAttribute<bool>());
	d_undone = false;
}

void ColumnClearCmd::undo()
{
	d_col->replaceData(d_data, d_validity);
	d_undone = true;
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnClearCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnClearValidityCmd
///////////////////////////////////////////////////////////////////////////
	ColumnClearValidityCmd::ColumnClearValidityCmd(shared_ptr<ColumnPrivate> col, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col)
{
	setText(QObject::tr("set column %1 valid").arg(col->columnLabel()));
	d_copied = false;
}

ColumnClearValidityCmd::~ColumnClearValidityCmd()
{
}

void ColumnClearValidityCmd::redo()
{
	if(!d_copied)
	{
		d_validity = d_col->validityAttribute();
		d_copied = true;
	}
	d_col->clearValidity();
}

void ColumnClearValidityCmd::undo()
{
	d_col->replaceData(d_col->dataPointer(), d_validity);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnClearValidityCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnClearMasksCmd
///////////////////////////////////////////////////////////////////////////
	ColumnClearMasksCmd::ColumnClearMasksCmd(shared_ptr<ColumnPrivate> col, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col)
{
	setText(QObject::tr("clear masks of column %1").arg(col->columnLabel()));
	d_copied = false;
}

ColumnClearMasksCmd::~ColumnClearMasksCmd()
{
}

void ColumnClearMasksCmd::redo()
{
	if(!d_copied)
	{
		d_masking = d_col->maskingAttribute();
		d_copied = true;
	}
	d_col->clearMasks();
}

void ColumnClearMasksCmd::undo()
{
	d_col->replaceMasking(d_masking);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnClearMasksCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnSetInvalidCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetInvalidCmd::ColumnSetInvalidCmd(shared_ptr<ColumnPrivate> col, Interval<int> interval, bool invalid, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_interval(interval), d_invalid(invalid)
{
	if(invalid)
		setText(QObject::tr("set cells invalid"));
	else
		setText(QObject::tr("set cells valid"));
	d_copied = false;
}

ColumnSetInvalidCmd::~ColumnSetInvalidCmd()
{
}

void ColumnSetInvalidCmd::redo()
{
	if(!d_copied)
	{
		d_validity = d_col->validityAttribute();
		d_copied = true;
	}
	d_col->setInvalid(d_interval, d_invalid);
}

void ColumnSetInvalidCmd::undo()
{
	d_col->replaceData(d_col->dataPointer(), d_validity);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetInvalidCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnSetMaskedCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetMaskedCmd::ColumnSetMaskedCmd(shared_ptr<ColumnPrivate> col, Interval<int> interval, bool masked, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_interval(interval), d_masked(masked)
{
	if(masked)
		setText(QObject::tr("mask cells"));
	else
		setText(QObject::tr("unmask cells"));
	d_copied = false;
}

ColumnSetMaskedCmd::~ColumnSetMaskedCmd()
{
}

void ColumnSetMaskedCmd::redo()
{
	if(!d_copied)
	{
		d_masking = d_col->maskingAttribute();
		d_copied = true;
	}
	d_col->setMasked(d_interval, d_masked);
}

void ColumnSetMaskedCmd::undo()
{
	d_col->replaceMasking(d_masking);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetMaskedCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnSetFormulaCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetFormulaCmd::ColumnSetFormulaCmd(shared_ptr<ColumnPrivate> col, Interval<int> interval, QString formula, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_interval(interval), d_formula(formula)
{
	setText(QObject::tr("set the formula for cell(s)"));
	d_copied = false;
}

ColumnSetFormulaCmd::~ColumnSetFormulaCmd()
{
}

void ColumnSetFormulaCmd::redo()
{
	if(!d_copied)
	{
		d_formulas = d_col->formulaAttribute();
		d_copied = true;
	}
	d_col->setFormula(d_interval, d_formula);
}

void ColumnSetFormulaCmd::undo()
{
	d_col->replaceFormulas(d_formulas);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetFormulaCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnClearFormulasCmd
///////////////////////////////////////////////////////////////////////////
	ColumnClearFormulasCmd::ColumnClearFormulasCmd(shared_ptr<ColumnPrivate> col, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col)
{
	setText(QObject::tr("clear all formulas of column %1").arg(col->columnLabel()));
	d_copied = false;
}

ColumnClearFormulasCmd::~ColumnClearFormulasCmd()
{
}

void ColumnClearFormulasCmd::redo()
{
	if(!d_copied)
	{
		d_formulas = d_col->formulaAttribute();
		d_copied = true;
	}
	d_col->clearFormulas();
}

void ColumnClearFormulasCmd::undo()
{
	d_col->replaceFormulas(d_formulas);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnClearFormulasCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnSetTextCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetTextCmd::ColumnSetTextCmd(shared_ptr<ColumnPrivate> col, int row, const QString& new_value, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_row(row), d_new_value(new_value)
{
	setText(QObject::tr("set text for row %1 in column %2").arg(row).arg(col->columnLabel()));
}

ColumnSetTextCmd::~ColumnSetTextCmd()
{
}

void ColumnSetTextCmd::redo()
{
	d_old_value = d_col->textAt(d_row);
	d_row_count = d_col->rowCount();
	d_col->setTextAt(d_row, d_new_value);
}

void ColumnSetTextCmd::undo()
{
	d_col->setTextAt(d_row, d_old_value);
	d_col->resizeTo(d_row_count);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetTextCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnSetValueCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetValueCmd::ColumnSetValueCmd(shared_ptr<ColumnPrivate> col, int row, double new_value, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_row(row), d_new_value(new_value)
{
	setText(QObject::tr("set value for row %1 in column %2").arg(row).arg(col->columnLabel()));
}

ColumnSetValueCmd::~ColumnSetValueCmd()
{
}

void ColumnSetValueCmd::redo()
{
	d_old_value = d_col->valueAt(d_row);
	d_row_count = d_col->rowCount();
	d_col->setValueAt(d_row, d_new_value);
}

void ColumnSetValueCmd::undo()
{
	d_col->setValueAt(d_row, d_old_value);
	d_col->resizeTo(d_row_count);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetValueCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnSetDateTimeCmd
///////////////////////////////////////////////////////////////////////////
	ColumnSetDateTimeCmd::ColumnSetDateTimeCmd(shared_ptr<ColumnPrivate> col, int row, const QDateTime& new_value, QUndoCommand * parent )
: QUndoCommand( parent ), d_col(col), d_row(row), d_new_value(new_value)
{
	setText(QObject::tr("set value for row %1 in column %2").arg(row).arg(col->columnLabel()));
}

ColumnSetDateTimeCmd::~ColumnSetDateTimeCmd()
{
}

void ColumnSetDateTimeCmd::redo()
{
	d_old_value = d_col->dateTimeAt(d_row);
	d_row_count = d_col->rowCount();
	d_col->setDateTimeAt(d_row, d_new_value);
}

void ColumnSetDateTimeCmd::undo()
{
	d_col->setDateTimeAt(d_row, d_old_value);
	d_col->resizeTo(d_row_count);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnSetDateTimeCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnReplaceTextsCmd
///////////////////////////////////////////////////////////////////////////
ColumnReplaceTextsCmd::ColumnReplaceTextsCmd(shared_ptr<ColumnPrivate> col, int first, const QStringList& new_values, QUndoCommand * parent )
 : QUndoCommand( parent ), d_col(col), d_first(first), d_new_values(new_values)
{
	setText(QObject::tr("replace the texts for rows %1 to %2 in column %3").arg(first).arg(first + new_values.count() -1).arg(col->columnLabel()));
	d_copied = false;
}

ColumnReplaceTextsCmd::~ColumnReplaceTextsCmd()
{
}

void ColumnReplaceTextsCmd::redo()
{
	if(!d_copied)
	{
		d_old_values = static_cast< QStringList* >(d_col->dataPointer())->mid(d_first, d_new_values.count());
		d_row_count = d_col->rowCount();
		d_copied = true;
	}
	d_col->replaceTexts(d_first, d_new_values);
}

void ColumnReplaceTextsCmd::undo()
{
	d_col->replaceTexts(d_first, d_old_values);
	d_col->resizeTo(d_row_count);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnReplaceTextsCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnReplaceValuesCmd
///////////////////////////////////////////////////////////////////////////
ColumnReplaceValuesCmd::ColumnReplaceValuesCmd(shared_ptr<ColumnPrivate> col, int first, const QVector<double>& new_values, QUndoCommand * parent )
 : QUndoCommand( parent ), d_col(col), d_first(first), d_new_values(new_values)
{
	setText(QObject::tr("replace the values for rows %1 to %2 in column %3").arg(first).arg(first + new_values.count() -1).arg(col->columnLabel()));
	d_copied = false;
}

ColumnReplaceValuesCmd::~ColumnReplaceValuesCmd()
{
}

void ColumnReplaceValuesCmd::redo()
{
	if(!d_copied)
	{
		d_old_values = static_cast< QVector<double>* >(d_col->dataPointer())->mid(d_first, d_new_values.count());
		d_row_count = d_col->rowCount();
		d_copied = true;
	}
	d_col->replaceValues(d_first, d_new_values);
}

void ColumnReplaceValuesCmd::undo()
{
	d_col->replaceValues(d_first, d_old_values);
	d_col->resizeTo(d_row_count);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnReplaceValuesCmd
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class ColumnReplaceDateTimesCmd
///////////////////////////////////////////////////////////////////////////
ColumnReplaceDateTimesCmd::ColumnReplaceDateTimesCmd(shared_ptr<ColumnPrivate> col, int first, const QList<QDateTime>& new_values, QUndoCommand * parent )
 : QUndoCommand( parent ), d_col(col), d_first(first), d_new_values(new_values)
{
	setText(QObject::tr("replace the values for rows %1 to %2 in column %3").arg(first).arg(first + new_values.count() -1).arg(col->columnLabel()));
	d_copied = false;
}

ColumnReplaceDateTimesCmd::~ColumnReplaceDateTimesCmd()
{
}

void ColumnReplaceDateTimesCmd::redo()
{
	if(!d_copied)
	{
		d_old_values = static_cast< QList<QDateTime>* >(d_col->dataPointer())->mid(d_first, d_new_values.count());
		d_row_count = d_col->rowCount();
		d_copied = true;
	}
	d_col->replaceDateTimes(d_first, d_new_values);
}

void ColumnReplaceDateTimesCmd::undo()
{
	d_col->replaceDateTimes(d_first, d_old_values);
	d_col->resizeTo(d_row_count);
}

///////////////////////////////////////////////////////////////////////////
// end of class ColumnReplaceDateTimesCmd
///////////////////////////////////////////////////////////////////////////


