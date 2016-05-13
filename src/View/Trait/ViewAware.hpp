/*
 * Copyright (C) 2010-2011 Daniel Richter <danielrichter2007@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef VIEW_VIEWAWARE_H_
#define VIEW_VIEWAWARE_H_

template <typename T>
class View_Trait_ViewAware {
	protected: std::shared_ptr<T> view;

	public: virtual ~View_Trait_ViewAware(){}

	public: void setView(std::shared_ptr<T> view) {
		this->view = view;
		this->initViewEvents();
	}

	public: virtual void initViewEvents()
	{
		// override to initialize specific view events
	}
};

#endif /* VIEW_VIEWAWARE_H_ */
